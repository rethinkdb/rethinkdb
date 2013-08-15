
$(BUILD_DIR)/rethinkdb-%.py: $(BACKUP_SCRIPTS_DIR)/rethinkdb-%.py
	$P CP
	cp $< $@

$(BACKUP_SCRIPTS_PROXY): $(BACKUP_SCRIPTS_DIR)/proxy.sh
	$P CP
	cp $< $@

.PHONY: backup-scripts
backup-scripts: $(BACKUP_SCRIPTS_REAL) $(BACKUP_SCRIPTS_PROXY)
