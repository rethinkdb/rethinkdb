// Copyright 2013 the V8 project authors. All rights reserved.
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
//       notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above
//       copyright notice, this list of conditions and the following
//       disclaimer in the documentation and/or other materials provided
//       with the distribution.
//     * Neither the name of Google Inc. nor the names of its
//       contributors may be used to endorse or promote products derived
//       from this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#include "hydrogen-alias-analysis.h"
#include "hydrogen-load-elimination.h"
#include "hydrogen-instructions.h"
#include "hydrogen-flow-engine.h"

namespace v8 {
namespace internal {

#define GLOBAL true
#define TRACE(x) if (FLAG_trace_load_elimination) PrintF x

static const int kMaxTrackedFields = 16;
static const int kMaxTrackedObjects = 5;

// An element in the field approximation list.
class HFieldApproximation : public ZoneObject {
 public:  // Just a data blob.
  HValue* object_;
  HLoadNamedField* last_load_;
  HValue* last_value_;
  HFieldApproximation* next_;

  // Recursively copy the entire linked list of field approximations.
  HFieldApproximation* Copy(Zone* zone) {
    if (this == NULL) return NULL;
    HFieldApproximation* copy = new(zone) HFieldApproximation();
    copy->object_ = this->object_;
    copy->last_load_ = this->last_load_;
    copy->last_value_ = this->last_value_;
    copy->next_ = this->next_->Copy(zone);
    return copy;
  }
};


// The main datastructure used during load/store elimination. Each in-object
// field is tracked separately. For each field, store a list of known field
// values for known objects.
class HLoadEliminationTable : public ZoneObject {
 public:
  HLoadEliminationTable(Zone* zone, HAliasAnalyzer* aliasing)
    : zone_(zone), fields_(kMaxTrackedFields, zone), aliasing_(aliasing) { }

  // The main processing of instructions.
  HLoadEliminationTable* Process(HInstruction* instr, Zone* zone) {
    switch (instr->opcode()) {
      case HValue::kLoadNamedField: {
        HLoadNamedField* l = HLoadNamedField::cast(instr);
        TRACE((" process L%d field %d (o%d)\n",
               instr->id(),
               FieldOf(l->access()),
               l->object()->ActualValue()->id()));
        HValue* result = load(l);
        if (result != instr) {
          // The load can be replaced with a previous load or a value.
          TRACE(("  replace L%d -> v%d\n", instr->id(), result->id()));
          instr->DeleteAndReplaceWith(result);
        }
        break;
      }
      case HValue::kStoreNamedField: {
        HStoreNamedField* s = HStoreNamedField::cast(instr);
        TRACE((" process S%d field %d (o%d) = v%d\n",
               instr->id(),
               FieldOf(s->access()),
               s->object()->ActualValue()->id(),
               s->value()->id()));
        HValue* result = store(s);
        if (result == NULL) {
          // The store is redundant. Remove it.
          TRACE(("  remove S%d\n", instr->id()));
          instr->DeleteAndReplaceWith(NULL);
        }
        break;
      }
      default: {
        if (instr->CheckGVNFlag(kChangesInobjectFields)) {
          TRACE((" kill-all i%d\n", instr->id()));
          Kill();
          break;
        }
        if (instr->CheckGVNFlag(kChangesMaps)) {
          TRACE((" kill-maps i%d\n", instr->id()));
          KillOffset(JSObject::kMapOffset);
        }
        if (instr->CheckGVNFlag(kChangesElementsKind)) {
          TRACE((" kill-elements-kind i%d\n", instr->id()));
          KillOffset(JSObject::kMapOffset);
          KillOffset(JSObject::kElementsOffset);
        }
        if (instr->CheckGVNFlag(kChangesElementsPointer)) {
          TRACE((" kill-elements i%d\n", instr->id()));
          KillOffset(JSObject::kElementsOffset);
        }
        if (instr->CheckGVNFlag(kChangesOsrEntries)) {
          TRACE((" kill-osr i%d\n", instr->id()));
          Kill();
        }
      }
      // Improvements possible:
      // - learn from HCheckMaps for field 0
      // - remove unobservable stores (write-after-write)
      // - track cells
      // - track globals
      // - track roots
    }
    return this;
  }

  // Support for global analysis with HFlowEngine: Copy state to sucessor block.
  HLoadEliminationTable* Copy(HBasicBlock* succ, Zone* zone) {
    HLoadEliminationTable* copy =
        new(zone) HLoadEliminationTable(zone, aliasing_);
    copy->EnsureFields(fields_.length());
    for (int i = 0; i < fields_.length(); i++) {
      copy->fields_[i] = fields_[i]->Copy(zone);
    }
    if (FLAG_trace_load_elimination) {
      TRACE((" copy-to B%d\n", succ->block_id()));
      copy->Print();
    }
    return copy;
  }

  // Support for global analysis with HFlowEngine: Merge this state with
  // the other incoming state.
  HLoadEliminationTable* Merge(HBasicBlock* succ,
      HLoadEliminationTable* that, Zone* zone) {
    if (that->fields_.length() < fields_.length()) {
      // Drop fields not in the other table.
      fields_.Rewind(that->fields_.length());
    }
    for (int i = 0; i < fields_.length(); i++) {
      // Merge the field approximations for like fields.
      HFieldApproximation* approx = fields_[i];
      HFieldApproximation* prev = NULL;
      while (approx != NULL) {
        // TODO(titzer): Merging is O(N * M); sort?
        HFieldApproximation* other = that->Find(approx->object_, i);
        if (other == NULL || !Equal(approx->last_value_, other->last_value_)) {
          // Kill an entry that doesn't agree with the other value.
          if (prev != NULL) {
            prev->next_ = approx->next_;
          } else {
            fields_[i] = approx->next_;
          }
          approx = approx->next_;
          continue;
        }
        prev = approx;
        approx = approx->next_;
      }
    }
    return this;
  }

  friend class HLoadEliminationEffects;  // Calls Kill() and others.
  friend class HLoadEliminationPhase;

 private:
  // Process a load instruction, updating internal table state. If a previous
  // load or store for this object and field exists, return the new value with
  // which the load should be replaced. Otherwise, return {instr}.
  HValue* load(HLoadNamedField* instr) {
    int field = FieldOf(instr->access());
    if (field < 0) return instr;

    HValue* object = instr->object()->ActualValue();
    HFieldApproximation* approx = FindOrCreate(object, field);

    if (approx->last_value_ == NULL) {
      // Load is not redundant. Fill out a new entry.
      approx->last_load_ = instr;
      approx->last_value_ = instr;
      return instr;
    } else {
      // Eliminate the load. Reuse previously stored value or load instruction.
      return approx->last_value_;
    }
  }

  // Process a store instruction, updating internal table state. If a previous
  // store to the same object and field makes this store redundant (e.g. because
  // the stored values are the same), return NULL indicating that this store
  // instruction is redundant. Otherwise, return {instr}.
  HValue* store(HStoreNamedField* instr) {
    int field = FieldOf(instr->access());
    if (field < 0) return KillIfMisaligned(instr);

    HValue* object = instr->object()->ActualValue();
    HValue* value = instr->value();

    // Kill non-equivalent may-alias entries.
    KillFieldInternal(object, field, value);
    if (instr->has_transition()) {
      // A transition store alters the map of the object.
      // TODO(titzer): remember the new map (a constant) for the object.
      KillFieldInternal(object, FieldOf(JSObject::kMapOffset), NULL);
    }
    HFieldApproximation* approx = FindOrCreate(object, field);

    if (Equal(approx->last_value_, value)) {
      // The store is redundant because the field already has this value.
      return NULL;
    } else {
      // The store is not redundant. Update the entry.
      approx->last_load_ = NULL;
      approx->last_value_ = value;
      return instr;
    }
  }

  // Kill everything in this table.
  void Kill() {
    fields_.Rewind(0);
  }

  // Kill all entries matching the given offset.
  void KillOffset(int offset) {
    int field = FieldOf(offset);
    if (field >= 0 && field < fields_.length()) {
      fields_[field] = NULL;
    }
  }

  // Kill all entries aliasing the given store.
  void KillStore(HStoreNamedField* s) {
    int field = FieldOf(s->access());
    if (field >= 0) {
      KillFieldInternal(s->object()->ActualValue(), field, s->value());
    } else {
      KillIfMisaligned(s);
    }
  }

  // Kill multiple entries in the case of a misaligned store.
  HValue* KillIfMisaligned(HStoreNamedField* instr) {
    HObjectAccess access = instr->access();
    if (access.IsInobject()) {
      int offset = access.offset();
      if ((offset % kPointerSize) != 0) {
        // Kill the field containing the first word of the access.
        HValue* object = instr->object()->ActualValue();
        int field = offset / kPointerSize;
        KillFieldInternal(object, field, NULL);

        // Kill the next field in case of overlap.
        int size = kPointerSize;
        if (access.representation().IsByte()) size = 1;
        else if (access.representation().IsInteger32()) size = 4;
        int next_field = (offset + size - 1) / kPointerSize;
        if (next_field != field) KillFieldInternal(object, next_field, NULL);
      }
    }
    return instr;
  }

  // Find an entry for the given object and field pair.
  HFieldApproximation* Find(HValue* object, int field) {
    // Search for a field approximation for this object.
    HFieldApproximation* approx = fields_[field];
    while (approx != NULL) {
      if (aliasing_->MustAlias(object, approx->object_)) return approx;
      approx = approx->next_;
    }
    return NULL;
  }

  // Find or create an entry for the given object and field pair.
  HFieldApproximation* FindOrCreate(HValue* object, int field) {
    EnsureFields(field + 1);

    // Search for a field approximation for this object.
    HFieldApproximation* approx = fields_[field];
    int count = 0;
    while (approx != NULL) {
      if (aliasing_->MustAlias(object, approx->object_)) return approx;
      count++;
      approx = approx->next_;
    }

    if (count >= kMaxTrackedObjects) {
      // Pull the last entry off the end and repurpose it for this object.
      approx = ReuseLastApproximation(field);
    } else {
      // Allocate a new entry.
      approx = new(zone_) HFieldApproximation();
    }

    // Insert the entry at the head of the list.
    approx->object_ = object;
    approx->last_load_ = NULL;
    approx->last_value_ = NULL;
    approx->next_ = fields_[field];
    fields_[field] = approx;

    return approx;
  }

  // Kill all entries for a given field that _may_ alias the given object
  // and do _not_ have the given value.
  void KillFieldInternal(HValue* object, int field, HValue* value) {
    if (field >= fields_.length()) return;  // Nothing to do.

    HFieldApproximation* approx = fields_[field];
    HFieldApproximation* prev = NULL;
    while (approx != NULL) {
      if (aliasing_->MayAlias(object, approx->object_)) {
        if (!Equal(approx->last_value_, value)) {
          // Kill an aliasing entry that doesn't agree on the value.
          if (prev != NULL) {
            prev->next_ = approx->next_;
          } else {
            fields_[field] = approx->next_;
          }
          approx = approx->next_;
          continue;
        }
      }
      prev = approx;
      approx = approx->next_;
    }
  }

  bool Equal(HValue* a, HValue* b) {
    if (a == b) return true;
    if (a != NULL && b != NULL) return a->Equals(b);
    return false;
  }

  // Remove the last approximation for a field so that it can be reused.
  // We reuse the last entry because it was the first inserted and is thus
  // farthest away from the current instruction.
  HFieldApproximation* ReuseLastApproximation(int field) {
    HFieldApproximation* approx = fields_[field];
    ASSERT(approx != NULL);

    HFieldApproximation* prev = NULL;
    while (approx->next_ != NULL) {
      prev = approx;
      approx = approx->next_;
    }
    if (prev != NULL) prev->next_ = NULL;
    return approx;
  }

  // Compute the field index for the given object access; -1 if not tracked.
  int FieldOf(HObjectAccess access) {
    return access.IsInobject() ? FieldOf(access.offset()) : -1;
  }

  // Compute the field index for the given in-object offset; -1 if not tracked.
  int FieldOf(int offset) {
    if (offset >= kMaxTrackedFields * kPointerSize) return -1;
    // TODO(titzer): track misaligned loads in a separate list?
    if ((offset % kPointerSize) != 0) return -1;  // Ignore misaligned accesses.
    return offset / kPointerSize;
  }

  // Ensure internal storage for the given number of fields.
  void EnsureFields(int num_fields) {
    if (fields_.length() < num_fields) {
      fields_.AddBlock(NULL, num_fields - fields_.length(), zone_);
    }
  }

  // Print this table to stdout.
  void Print() {
    for (int i = 0; i < fields_.length(); i++) {
      PrintF("  field %d: ", i);
      for (HFieldApproximation* a = fields_[i]; a != NULL; a = a->next_) {
        PrintF("[o%d =", a->object_->id());
        if (a->last_load_ != NULL) PrintF(" L%d", a->last_load_->id());
        if (a->last_value_ != NULL) PrintF(" v%d", a->last_value_->id());
        PrintF("] ");
      }
      PrintF("\n");
    }
  }

  Zone* zone_;
  ZoneList<HFieldApproximation*> fields_;
  HAliasAnalyzer* aliasing_;
};


// Support for HFlowEngine: collect store effects within loops.
class HLoadEliminationEffects : public ZoneObject {
 public:
  explicit HLoadEliminationEffects(Zone* zone)
    : zone_(zone),
      maps_stored_(false),
      fields_stored_(false),
      elements_stored_(false),
      stores_(5, zone) { }

  inline bool Disabled() {
    return false;  // Effects are _not_ disabled.
  }

  // Process a possibly side-effecting instruction.
  void Process(HInstruction* instr, Zone* zone) {
    switch (instr->opcode()) {
      case HValue::kStoreNamedField: {
        stores_.Add(HStoreNamedField::cast(instr), zone_);
        break;
      }
      case HValue::kOsrEntry: {
        // Kill everything. Loads must not be hoisted past the OSR entry.
        maps_stored_ = true;
        fields_stored_ = true;
        elements_stored_ = true;
      }
      default: {
        fields_stored_ |= instr->CheckGVNFlag(kChangesInobjectFields);
        maps_stored_ |= instr->CheckGVNFlag(kChangesMaps);
        maps_stored_ |= instr->CheckGVNFlag(kChangesElementsKind);
        elements_stored_ |= instr->CheckGVNFlag(kChangesElementsKind);
        elements_stored_ |= instr->CheckGVNFlag(kChangesElementsPointer);
      }
    }
  }

  // Apply these effects to the given load elimination table.
  void Apply(HLoadEliminationTable* table) {
    if (fields_stored_) {
      table->Kill();
      return;
    }
    if (maps_stored_) {
      table->KillOffset(JSObject::kMapOffset);
    }
    if (elements_stored_) {
      table->KillOffset(JSObject::kElementsOffset);
    }

    // Kill non-agreeing fields for each store contained in these effects.
    for (int i = 0; i < stores_.length(); i++) {
      table->KillStore(stores_[i]);
    }
  }

  // Union these effects with the other effects.
  void Union(HLoadEliminationEffects* that, Zone* zone) {
    maps_stored_ |= that->maps_stored_;
    fields_stored_ |= that->fields_stored_;
    elements_stored_ |= that->elements_stored_;
    for (int i = 0; i < that->stores_.length(); i++) {
      stores_.Add(that->stores_[i], zone);
    }
  }

 private:
  Zone* zone_;
  bool maps_stored_ : 1;
  bool fields_stored_ : 1;
  bool elements_stored_ : 1;
  ZoneList<HStoreNamedField*> stores_;
};


// The main routine of the analysis phase. Use the HFlowEngine for either a
// local or a global analysis.
void HLoadEliminationPhase::Run() {
  HFlowEngine<HLoadEliminationTable, HLoadEliminationEffects>
    engine(graph(), zone());
  HAliasAnalyzer aliasing;
  HLoadEliminationTable* table =
      new(zone()) HLoadEliminationTable(zone(), &aliasing);

  if (GLOBAL) {
    // Perform a global analysis.
    engine.AnalyzeDominatedBlocks(graph()->blocks()->at(0), table);
  } else {
    // Perform only local analysis.
    for (int i = 0; i < graph()->blocks()->length(); i++) {
      table->Kill();
      engine.AnalyzeOneBlock(graph()->blocks()->at(i), table);
    }
  }
}

} }  // namespace v8::internal
