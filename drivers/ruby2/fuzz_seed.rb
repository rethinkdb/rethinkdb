[(r("test").db_create),
 (r.db("test").table_create("tbl")),
 (r.db("test").table("tbl").delete),
 (r.db("test").table("tbl").insert(
   [{:num => 0, :name => "0", :id => 0},
    {:num => 1, :name => "1", :id => 1},
    {:num => 2, :name => "2", :id => 2},
    {:num => 3, :name => "3", :id => 3},
    {:num => 4, :name => "4", :id => 4},
    {:num => 5, :name => "5", :id => 5},
    {:num => 6, :name => "6", :id => 6},
    {:num => 7, :name => "7", :id => 7},
    {:num => 8, :name => "8", :id => 8},
    {:num => 9, :name => "9", :id => 9}]
 )),
 (r([1, 2, 3]).lt([1, 2, 3, 4])),
 (r([1, 2, 3]).lt([1, 2, 3])),
 (r("test").db_create),
 (r.db("test").table_create("tbl")),
 (r.db("test").table("tbl").delete),
 (r.db("test").table("tbl").insert(
   [{:num => 0, :name => "0", :id => 0},
    {:num => 1, :name => "1", :id => 1},
    {:num => 2, :name => "2", :id => 2},
    {:num => 3, :name => "3", :id => 3},
    {:num => 4, :name => "4", :id => 4},
    {:num => 5, :name => "5", :id => 5},
    {:num => 6, :name => "6", :id => 6},
    {:num => 7, :name => "7", :id => 7},
    {:num => 8, :name => "8", :id => 8},
    {:num => 9, :name => "9", :id => 9}]
 )),
 (r([true, false, nil])),
 (r([true, false, nil]).coerce_to("array")),
 (r("test").db_create),
 (r.db("test").table_create("tbl")),
 (r.db("test").table("tbl").delete),
 (r.db("test").table("tbl").insert(
   [{:num => 0, :name => "0", :id => 0},
    {:num => 1, :name => "1", :id => 1},
    {:num => 2, :name => "2", :id => 2},
    {:num => 3, :name => "3", :id => 3},
    {:num => 4, :name => "4", :id => 4},
    {:num => 5, :name => "5", :id => 5},
    {:num => 6, :name => "6", :id => 6},
    {:num => 7, :name => "7", :id => 7},
    {:num => 8, :name => "8", :id => 8},
    {:num => 9, :name => "9", :id => 9}]
 )),
 (r.db("test").table("tbl")),
 (r([2, 3, 4]).foreach {|var_1|
   [r.db("test").table("tbl").get(var_1).update { {:num => 0}},
    r.db("test").table("tbl").get(var_1.mul(2)).update { {:num => 0}}]
 }),
 (r.db("test").table("tbl").coerce_to("array").foreach {|var_2|
   r.db("test").table("tbl").filter {|var_3|
     var_3["id"].eq(var_2["id"])
   }.update {|var_4| var_4["num"].eq(0).branch({:num => var_4["id"]}, nil)}
 }),
 (r.db("test").table("tbl")),
 (r("test").db_create),
 (r.db("test").table_create("tbl")),
 (r.db("test").table("tbl").delete),
 (r.db("test").table("tbl").insert(
   [{:num => 0, :name => "0", :id => 0},
    {:num => 1, :name => "1", :id => 1},
    {:num => 2, :name => "2", :id => 2},
    {:num => 3, :name => "3", :id => 3},
    {:num => 4, :name => "4", :id => 4},
    {:num => 5, :name => "5", :id => 5},
    {:num => 6, :name => "6", :id => 6},
    {:num => 7, :name => "7", :id => 7},
    {:num => 8, :name => "8", :id => 8},
    {:num => 9, :name => "9", :id => 9}]
 )),
 (r([]).append(2)),
 (r([1]).append(2)),
 (r(2).append(0)),
 (r([1]).union([2])),
 (r([1, 2]).union([])),
 (r(1).union([1])),
 (r([1]).union(1)),
 (r([0, 1, 2, 3, 4, 5, 6, 7, 8, 9]).slice(0, 2)),
 (r([0, 1, 2, 3, 4, 5, 6, 7, 8, 9]).slice(5, 14)),
 (r([0, 1, 2, 3, 4, 5, 6, 7, 8, 9]).slice(5, -4)),
 (r([0, 1, 2, 3, 4, 5, 6, 7, 8, 9]).slice(-5, -4)),
 (r([0, 1, 2, 3, 4, 5, 6, 7, 8, 9]).slice(0, 3)),
 (r([0, 1, 2, 3, 4, 5, 6, 7, 8, 9]).slice(0, 0)),
 (r([0, 1, 2, 3, 4, 5, 6, 7, 8, 9]).slice(5, 15)),
 (r([0, 1, 2, 3, 4, 5, 6, 7, 8, 9]).slice(5, -3)),
 (r([0, 1, 2, 3, 4, 5, 6, 7, 8, 9]).slice(-5, -3)),
 (r(1).slice(0, 0)),
 (r([0, 1, 2, 3, 4, 5, 6, 7, 8, 9]).slice(0.5, 0)),
 (r(1).slice(0, 0.01)),
 (r(1).slice(5, 2)),
 (r([0, 1, 2, 3, 4, 5, 6, 7, 8, 9]).slice(5, -1)),
 (r([0, 1, 2, 3, 4, 5, 6, 7, 8, 9]).slice(0, 6)),
 (r([0, 1, 2, 3, 4, 5, 6, 7, 8, 9]).slice(0, -3)),
 (r([0, 1, 2, 3, 4, 5, 6, 7, 8, 9]).slice(-2, -1)),
 (r([0, 1, 2, 3, 4, 5, 6, 7, 8, 9]).slice(0, -1)),
 (r([0, 1, 2, 3, 4, 5, 6, 7, 8, 9]).nth(3)),
 (r([0, 1, 2, 3, 4, 5, 6, 7, 8, 9]).nth(-1)),
 (r(0).nth(0)),
 (r([0]).nth(1)),
 (r([]).count),
 (r([0, 1, 2, 3, 4, 5, 6, 7, 8, 9]).count),
 (r(0).count),
 (r("test").db_create),
 (r.db("test").table_create("tbl")),
 (r.db("test").table("tbl").delete),
 (r.db("test").table("tbl").insert(
   [{:num => 0, :name => "0", :id => 0},
    {:num => 1, :name => "1", :id => 1},
    {:num => 2, :name => "2", :id => 2},
    {:num => 3, :name => "3", :id => 3},
    {:num => 4, :name => "4", :id => 4},
    {:num => 5, :name => "5", :id => 5},
    {:num => 6, :name => "6", :id => 6},
    {:num => 7, :name => "7", :id => 7},
    {:num => 8, :name => "8", :id => 8},
    {:num => 9, :name => "9", :id => 9}]
 )),
 (r.db("test").table("tbl").insert({:id => []})),
 (r.db("test").table("tbl").get(100).replace { {:id => []}}),
 (r("test").db_create),
 (r.db("test").table_create("tbl")),
 (r.db("test").table("tbl").delete),
 (r.db("test").table("tbl").insert(
   [{:num => 0, :name => "0", :id => 0},
    {:num => 1, :name => "1", :id => 1},
    {:num => 2, :name => "2", :id => 2},
    {:num => 3, :name => "3", :id => 3},
    {:num => 4, :name => "4", :id => 4},
    {:num => 5, :name => "5", :id => 5},
    {:num => 6, :name => "6", :id => 6},
    {:num => 7, :name => "7", :id => 7},
    {:num => 8, :name => "8", :id => 8},
    {:num => 9, :name => "9", :id => 9}]
 )),
 (r.db("test").table("tbl").insert(
   [{:id => 0},
    {:id => 1},
    {:id => 2},
    {:id => 3},
    {:id => 4},
    {:id => 5},
    {:id => 6},
    {:id => 7},
    {:id => 8},
    {:id => 9},
    {:id => 10},
    {:id => 11},
    {:id => 12},
    {:id => 13},
    {:id => 14},
    {:id => 15},
    {:id => 16},
    {:id => 17},
    {:id => 18},
    {:id => 19},
    {:id => 20},
    {:id => 21},
    {:id => 22},
    {:id => 23},
    {:id => 24},
    {:id => 25},
    {:id => 26},
    {:id => 27},
    {:id => 28},
    {:id => 29},
    {:id => 30},
    {:id => 31},
    {:id => 32},
    {:id => 33},
    {:id => 34},
    {:id => 35},
    {:id => 36},
    {:id => 37},
    {:id => 38},
    {:id => 39},
    {:id => 40},
    {:id => 41},
    {:id => 42},
    {:id => 43},
    {:id => 44},
    {:id => 45},
    {:id => 46},
    {:id => 47},
    {:id => 48},
    {:id => 49},
    {:id => 50},
    {:id => 51},
    {:id => 52},
    {:id => 53},
    {:id => 54},
    {:id => 55},
    {:id => 56},
    {:id => 57},
    {:id => 58},
    {:id => 59},
    {:id => 60},
    {:id => 61},
    {:id => 62},
    {:id => 63},
    {:id => 64},
    {:id => 65},
    {:id => 66},
    {:id => 67},
    {:id => 68},
    {:id => 69},
    {:id => 70},
    {:id => 71},
    {:id => 72},
    {:id => 73},
    {:id => 74},
    {:id => 75},
    {:id => 76},
    {:id => 77},
    {:id => 78},
    {:id => 79},
    {:id => 80},
    {:id => 81},
    {:id => 82},
    {:id => 83},
    {:id => 84},
    {:id => 85},
    {:id => 86},
    {:id => 87},
    {:id => 88},
    {:id => 89},
    {:id => 90},
    {:id => 91},
    {:id => 92},
    {:id => 93},
    {:id => 94},
    {:id => 95},
    {:id => 96},
    {:id => 97},
    {:id => 98},
    {:id => 99}]
 )),
 (r.db("test").table("tbl").between({:left_bound => 1, :right_bound => 2})),
 (r.db("test").table("tbl").delete),
 (r.db("test").table("tbl").insert(
   [{:num => 0, :name => "0", :id => 0},
    {:num => 1, :name => "1", :id => 1},
    {:num => 2, :name => "2", :id => 2},
    {:num => 3, :name => "3", :id => 3},
    {:num => 4, :name => "4", :id => 4},
    {:num => 5, :name => "5", :id => 5},
    {:num => 6, :name => "6", :id => 6},
    {:num => 7, :name => "7", :id => 7},
    {:num => 8, :name => "8", :id => 8},
    {:num => 9, :name => "9", :id => 9}]
 )),
 (r.db("test").table("tbl")),
 (r("test").db_create),
 (r.db("test").table_create("tbl")),
 (r.db("test").table("tbl").delete),
 (r.db("test").table("tbl").insert(
   [{:num => 0, :name => "0", :id => 0},
    {:num => 1, :name => "1", :id => 1},
    {:num => 2, :name => "2", :id => 2},
    {:num => 3, :name => "3", :id => 3},
    {:num => 4, :name => "4", :id => 4},
    {:num => 5, :name => "5", :id => 5},
    {:num => 6, :name => "6", :id => 6},
    {:num => 7, :name => "7", :id => 7},
    {:num => 8, :name => "8", :id => 8},
    {:num => 9, :name => "9", :id => 9}]
 )),
 (r("test").db_create),
 (r.db("test").table_create("tbl")),
 (r.db("test").table("tbl").delete),
 (r.db("test").table("tbl").insert(
   [{:num => 0, :name => "0", :id => 0},
    {:num => 1, :name => "1", :id => 1},
    {:num => 2, :name => "2", :id => 2},
    {:num => 3, :name => "3", :id => 3},
    {:num => 4, :name => "4", :id => 4},
    {:num => 5, :name => "5", :id => 5},
    {:num => 6, :name => "6", :id => 6},
    {:num => 7, :name => "7", :id => 7},
    {:num => 8, :name => "8", :id => 8},
    {:num => 9, :name => "9", :id => 9}]
 )),
 (r(3).eq(3)),
 (r(3).eq(4)),
 (r(3).ne(3)),
 (r(3).ne(4)),
 (r(3).gt(2)),
 (r(3).gt(3)),
 (r(3).ge(3)),
 (r(3).ge(4)),
 (r(3).lt(3)),
 (r(3).lt(4)),
 (r(3).le(3)),
 (r(3).le(4)),
 (r(3).le(2)),
 (r(1).eq(1, 2)),
 (r(5).eq(5, 5)),
 (r(3).lt(4)),
 (r(3).lt(4, 5)),
 (r(5).lt(6, 7, 7)),
 (r("asdf").eq("asdf")),
 (r("asd").eq("asdf")),
 (r("a").lt("b")),
 (r(true).eq(true)),
 (r(false).lt(true)),
 (r([]).lt(false, true, nil, 1, {}, "")),
 (r("test").db_create),
 (r.db("test").table_create("tbl")),
 (r.db("test").table("tbl").delete),
 (r.db("test").table("tbl").insert(
   [{:num => 0, :name => "0", :id => 0},
    {:num => 1, :name => "1", :id => 1},
    {:num => 2, :name => "2", :id => 2},
    {:num => 3, :name => "3", :id => 3},
    {:num => 4, :name => "4", :id => 4},
    {:num => 5, :name => "5", :id => 5},
    {:num => 6, :name => "6", :id => 6},
    {:num => 7, :name => "7", :id => 7},
    {:num => 8, :name => "8", :id => 8},
    {:num => 9, :name => "9", :id => 9}]
 )),
 (r(1).coerce_to("string")),
 (r({:a => 1, :b => 2}).coerce_to("array")),
 (r(1).coerce_to("datum")),
 (r.db("test").table("tbl").coerce_to("array").slice(-3, -2)),
 (r.db("test").table("tbl").slice(-3, -2)),
 (r([["a", 1], ["b", 2]]).coerce_to("object")),
 (r([["a", 1], ["a", 2]]).coerce_to("object")),
 (r("test").db_create),
 (r.db("test").table_create("tbl")),
 (r.db("test").table("tbl").delete),
 (r.db("test").table("tbl").insert(
   [{:num => 0, :name => "0", :id => 0},
    {:num => 1, :name => "1", :id => 1},
    {:num => 2, :name => "2", :id => 2},
    {:num => 3, :name => "3", :id => 3},
    {:num => 4, :name => "4", :id => 4},
    {:num => 5, :name => "5", :id => 5},
    {:num => 6, :name => "6", :id => 6},
    {:num => 7, :name => "7", :id => 7},
    {:num => 8, :name => "8", :id => 8},
    {:num => 9, :name => "9", :id => 9}]
 )),
 (r([1, 2, 3]).concatmap { [1, 2]}),
 (r([[1], [2]]).concatmap {|var_5| var_5}),
 (r([[1], 2]).concatmap {|var_6| var_6}),
 (r(1).concatmap {|var_7| var_7}),
 (r([[1], [2]]).concatmap {|var_8| var_8}),
 (r.db("test").table("tbl").concatmap {|var_9|
   r.db("test").table("tbl").map {|var_10| var_10["id"].mul(var_9["id"])}
 }.distinct),
 (r("test").db_create),
 (r.db("test").table_create("tbl")),
 (r.db("test").table("tbl").delete),
 (r.db("test").table("tbl").insert(
   [{:num => 0, :name => "0", :id => 0},
    {:num => 1, :name => "1", :id => 1},
    {:num => 2, :name => "2", :id => 2},
    {:num => 3, :name => "3", :id => 3},
    {:num => 4, :name => "4", :id => 4},
    {:num => 5, :name => "5", :id => 5},
    {:num => 6, :name => "6", :id => 6},
    {:num => 7, :name => "7", :id => 7},
    {:num => 8, :name => "8", :id => 8},
    {:num => 9, :name => "9", :id => 9}]
 )),
 (r({:a => 1, :b => 2}).contains("a")),
 (r({:a => 1, :b => 2}).contains("c")),
 (r({:a => 1, :b => 2}).contains("a", "b")),
 (r({:a => 1, :b => 2}).contains("a", "c")),
 (r("test").db_create),
 (r.db("test").table_create("tbl")),
 (r.db("test").table("tbl").delete),
 (r.db("test").table("tbl").insert(
   [{:num => 0, :name => "0", :id => 0},
    {:num => 1, :name => "1", :id => 1},
    {:num => 2, :name => "2", :id => 2},
    {:num => 3, :name => "3", :id => 3},
    {:num => 4, :name => "4", :id => 4},
    {:num => 5, :name => "5", :id => 5},
    {:num => 6, :name => "6", :id => 6},
    {:num => 7, :name => "7", :id => 7},
    {:num => 8, :name => "8", :id => 8},
    {:num => 9, :name => "9", :id => 9}]
 )),
 (lambda {|var_11| var_11}.funcall(3)),
 (lambda {|var_12, var_13| var_12}.funcall(3)),
 (lambda {|var_14, var_15| var_14}.funcall(3, 4)),
 (lambda {|var_16, var_17| var_17}.funcall(3, 4)),
 (lambda {|var_18, var_19| var_18.add(var_19)}.funcall(3, 4)),
 (r("test").db_create),
 (r.db("test").table_create("tbl")),
 (r.db("test").table("tbl").delete),
 (r.db("test").table("tbl").insert(
   [{:num => 0, :name => "0", :id => 0},
    {:num => 1, :name => "1", :id => 1},
    {:num => 2, :name => "2", :id => 2},
    {:num => 3, :name => "3", :id => 3},
    {:num => 4, :name => "4", :id => 4},
    {:num => 5, :name => "5", :id => 5},
    {:num => 6, :name => "6", :id => 6},
    {:num => 7, :name => "7", :id => 7},
    {:num => 8, :name => "8", :id => 8},
    {:num => 9, :name => "9", :id => 9}]
 )),
 (r.db("test").table("tbl")),
 (r.db("test").table("tbl")),
 (r("test").db_create),
 (r.db("test").table_create("tbl")),
 (r.db("test").table("tbl").delete),
 (r.db("test").table("tbl").insert(
   [{:num => 0, :name => "0", :id => 0},
    {:num => 1, :name => "1", :id => 1},
    {:num => 2, :name => "2", :id => 2},
    {:num => 3, :name => "3", :id => 3},
    {:num => 4, :name => "4", :id => 4},
    {:num => 5, :name => "5", :id => 5},
    {:num => 6, :name => "6", :id => 6},
    {:num => 7, :name => "7", :id => 7},
    {:num => 8, :name => "8", :id => 8},
    {:num => 9, :name => "9", :id => 9}]
 )),
 (r(1).add(2).ge(3).branch([1, 2, 3], r("unreachable").error)),
 (r(1).add(2).gt(3).branch([1, 2, 3], r("reachable").error)),
 (r("test").db_create),
 (r.db("test").table_create("tbl")),
 (r.db("test").table("tbl").delete),
 (r.db("test").table("tbl").insert(
   [{:num => 0, :name => "0", :id => 0},
    {:num => 1, :name => "1", :id => 1},
    {:num => 2, :name => "2", :id => 2},
    {:num => 3, :name => "3", :id => 3},
    {:num => 4, :name => "4", :id => 4},
    {:num => 5, :name => "5", :id => 5},
    {:num => 6, :name => "6", :id => 6},
    {:num => 7, :name => "7", :id => 7},
    {:num => 8, :name => "8", :id => 8},
    {:num => 9, :name => "9", :id => 9}]
 )),
 (r("7821881246295613570").db_create),
 (r.db("7821881246295613570").table_create("16522553843283354870")),
 (r.db("7821881246295613570").table("16522553843283354870").insert(
   [{:num => 0, :name => "0", :id => 0},
    {:num => 1, :name => "1", :id => 1},
    {:num => 2, :name => "2", :id => 2},
    {:num => 3, :name => "3", :id => 3},
    {:num => 4, :name => "4", :id => 4},
    {:num => 5, :name => "5", :id => 5},
    {:num => 6, :name => "6", :id => 6},
    {:num => 7, :name => "7", :id => 7},
    {:num => 8, :name => "8", :id => 8},
    {:num => 9, :name => "9", :id => 9}]
 )),
 (r.db("7821881246295613570").table("16522553843283354870").insert(
   {:broken => true, :id => 11}
 )),
 (r.db("7821881246295613570").table("16522553843283354870").insert(
   {:broken => true, :id => 12}
 )),
 (r.db("7821881246295613570").table("16522553843283354870").insert(
   {:broken => true, :id => 13}
 )),
 (r.db("7821881246295613570").table("16522553843283354870").orderby(
   "id"
 ).foreach {|var_20|
   [r.db("7821881246295613570").table("16522553843283354870").update {|var_21|
      var_20["id"].eq(var_21["id"]).branch({:num => var_21["num"].mul(2)}, nil)
    },
    r.db("7821881246295613570").table("16522553843283354870").filter {|var_22|
      var_22["id"].eq(var_20["id"])
    }.replace {|var_23| var_23["id"].mod(2).eq(1).branch(nil, var_23)},
    r.db("7821881246295613570").table("16522553843283354870").get(0).delete,
    r.db("7821881246295613570").table("16522553843283354870").get(12).delete]
 }),
 (r.db("7821881246295613570").table("16522553843283354870").orderby("id")),
 (r("7821881246295613570").db_drop),
 (r("test").db_create),
 (r.db("test").table_create("tbl")),
 (r.db("test").table("tbl").delete),
 (r.db("test").table("tbl").insert(
   [{:num => 0, :name => "0", :id => 0},
    {:num => 1, :name => "1", :id => 1},
    {:num => 2, :name => "2", :id => 2},
    {:num => 3, :name => "3", :id => 3},
    {:num => 4, :name => "4", :id => 4},
    {:num => 5, :name => "5", :id => 5},
    {:num => 6, :name => "6", :id => 6},
    {:num => 7, :name => "7", :id => 7},
    {:num => 8, :name => "8", :id => 8},
    {:num => 9, :name => "9", :id => 9}]
 )),
 (r([1, 2, 3]).filter {|var_24| var_24.lt(3)}),
 (r(1).filter { true}),
 (r.db("test").table("tbl").filter {|var_25| var_25["name"].eq("5")}),
 (r.db("test").table("tbl").filter {|var_26|
   var_26["id"].ge(2).all(var_26["id"].le(5))
 }),
 (r.db("test").table("tbl").filter {|var_26|
   var_26["id"].ge(2).all(var_26["id"].le(5))
 }.filter {|var_27| var_27["num"].ne(5)}),
 (r.db("test").table("tbl").filter {|var_26|
   var_26["id"].ge(2).all(var_26["id"].le(5))
 }.filter {|var_27| var_27["num"].ne(5)}.filter {|var_28|
   var_28["num"].eq(2).any(var_28["num"].eq(3))
 }),
 (r.db("test").table("tbl").filter {|var_29| var_29["id"].lt(5)}),
 (r("test").db_create),
 (r.db("test").table_create("tbl")),
 (r.db("test").table("tbl").delete),
 (r.db("test").table("tbl").insert(
   [{:num => 0, :name => "0", :id => 0},
    {:num => 1, :name => "1", :id => 1},
    {:num => 2, :name => "2", :id => 2},
    {:num => 3, :name => "3", :id => 3},
    {:num => 4, :name => "4", :id => 4},
    {:num => 5, :name => "5", :id => 5},
    {:num => 6, :name => "6", :id => 6},
    {:num => 7, :name => "7", :id => 7},
    {:num => 8, :name => "8", :id => 8},
    {:num => 9, :name => "9", :id => 9}]
 )),
 (r.db("test").table("tbl").orderby("id")),
 (r.db("test").table("tbl").foreach {|var_30|
   r.db("test").table("tbl").get(var_30["id"]).update { {:id => 0}}
 }),
 (r.db("test").table("tbl").orderby("id")),
 (r("test").db_create),
 (r.db("test").table_create("tbl")),
 (r.db("test").table("tbl").delete),
 (r.db("test").table("tbl").insert(
   [{:num => 0, :name => "0", :id => 0},
    {:num => 1, :name => "1", :id => 1},
    {:num => 2, :name => "2", :id => 2},
    {:num => 3, :name => "3", :id => 3},
    {:num => 4, :name => "4", :id => 4},
    {:num => 5, :name => "5", :id => 5},
    {:num => 6, :name => "6", :id => 6},
    {:num => 7, :name => "7", :id => 7},
    {:num => 8, :name => "8", :id => 8},
    {:num => 9, :name => "9", :id => 9}]
 )),
 (r("foreach_multi5230273763069510756").db_create),
 (r.db("foreach_multi5230273763069510756").table_create(
   "foreach_multi_tbl5147648423918806630"
 )),
 (r.db("foreach_multi5230273763069510756").table_create(
   "foreach_multi_tbl514764842391880663014403288229146751962"
 )),
 (r.db("foreach_multi5230273763069510756").table_create(
   "foreach_multi_tbl51476484239188066301440328822914675196211939607283717513698"
 )),
 (r.db("foreach_multi5230273763069510756").table_create(
   "foreach_multi_tbl514764842391880663014403288229146751962119396072837175136983517474372708758325"
 )),
 (r.db("foreach_multi5230273763069510756").table(
   "foreach_multi_tbl5147648423918806630"
 ).insert(
   [{:num => 0, :name => "0", :id => 0},
    {:num => 1, :name => "1", :id => 1},
    {:num => 2, :name => "2", :id => 2},
    {:num => 3, :name => "3", :id => 3},
    {:num => 4, :name => "4", :id => 4},
    {:num => 5, :name => "5", :id => 5},
    {:num => 6, :name => "6", :id => 6},
    {:num => 7, :name => "7", :id => 7},
    {:num => 8, :name => "8", :id => 8},
    {:num => 9, :name => "9", :id => 9}]
 )),
 (r.db("foreach_multi5230273763069510756").table(
   "foreach_multi_tbl514764842391880663014403288229146751962"
 ).insert(
   [{:num => 0, :name => "0", :id => 0},
    {:num => 1, :name => "1", :id => 1},
    {:num => 2, :name => "2", :id => 2},
    {:num => 3, :name => "3", :id => 3},
    {:num => 4, :name => "4", :id => 4},
    {:num => 5, :name => "5", :id => 5},
    {:num => 6, :name => "6", :id => 6},
    {:num => 7, :name => "7", :id => 7},
    {:num => 8, :name => "8", :id => 8},
    {:num => 9, :name => "9", :id => 9}]
 )),
 (r.db("foreach_multi5230273763069510756").table(
   "foreach_multi_tbl5147648423918806630"
 ).foreach {|var_31|
   r.db("foreach_multi5230273763069510756").table(
     "foreach_multi_tbl514764842391880663014403288229146751962"
   ).foreach {|var_32|
     r.db("foreach_multi5230273763069510756").table(
       "foreach_multi_tbl51476484239188066301440328822914675196211939607283717513698"
     ).insert({:id => var_31["id"].mul(1000).add(var_32["id"])})
   }
 }),
 (r.db("foreach_multi5230273763069510756").table(
   "foreach_multi_tbl5147648423918806630"
 ).foreach {|var_33|
   r.db("foreach_multi5230273763069510756").table(
     "foreach_multi_tbl514764842391880663014403288229146751962"
   ).filter {|var_34| var_34["id"].eq(var_33["id"].mul(var_33["id"]))}.delete
 }),
 (r.db("foreach_multi5230273763069510756").table(
   "foreach_multi_tbl5147648423918806630"
 ).foreach {|var_35|
   r.db("foreach_multi5230273763069510756").table(
     "foreach_multi_tbl514764842391880663014403288229146751962"
   ).foreach {|var_36|
     r.db("foreach_multi5230273763069510756").table(
       "foreach_multi_tbl51476484239188066301440328822914675196211939607283717513698"
     ).filter {|var_37|
       var_37["id"].eq(var_35["id"].mul(1000).add(var_36["id"]))
     }.delete
   }
 }),
 (r.db("foreach_multi5230273763069510756").table(
   "foreach_multi_tbl5147648423918806630"
 ).foreach {|var_38|
   r.db("foreach_multi5230273763069510756").table(
   "foreach_multi_tbl514764842391880663014403288229146751962119396072837175136983517474372708758325"
   ).insert(var_38)
 }),
 (r.db("foreach_multi5230273763069510756").table(
   "foreach_multi_tbl514764842391880663014403288229146751962"
 ).foreach {|var_39|
   r.db("foreach_multi5230273763069510756").table(
     "foreach_multi_tbl514764842391880663014403288229146751962119396072837175136983517474372708758325"
   ).filter {|var_40| var_40["id"].eq(var_39["id"])}.delete
 }),
 (r("foreach_multi5230273763069510756").db_drop),
 (r("test").db_create),
 (r.db("test").table_create("tbl")),
 (r.db("test").table("tbl").delete),
 (r.db("test").table("tbl").insert(
   [{:num => 0, :name => "0", :id => 0},
    {:num => 1, :name => "1", :id => 1},
    {:num => 2, :name => "2", :id => 2},
    {:num => 3, :name => "3", :id => 3},
    {:num => 4, :name => "4", :id => 4},
    {:num => 5, :name => "5", :id => 5},
    {:num => 6, :name => "6", :id => 6},
    {:num => 7, :name => "7", :id => 7},
    {:num => 8, :name => "8", :id => 8},
    {:num => 9, :name => "9", :id => 9}]
 )),
 (r({:obj => r.db("test").table("tbl").get(0)})),
 (r({:obj => r.db("test").table("tbl").get(0)})),
 (r("test").db_create),
 (r.db("test").table_create("tbl")),
 (r.db("test").table("tbl").delete),
 (r.db("test").table("tbl").insert(
   [{:num => 0, :name => "0", :id => 0},
    {:num => 1, :name => "1", :id => 1},
    {:num => 2, :name => "2", :id => 2},
    {:num => 3, :name => "3", :id => 3},
    {:num => 4, :name => "4", :id => 4},
    {:num => 5, :name => "5", :id => 5},
    {:num => 6, :name => "6", :id => 6},
    {:num => 7, :name => "7", :id => 7},
    {:num => 8, :name => "8", :id => 8},
    {:num => 9, :name => "9", :id => 9}]
 )),
 (r([0, 1, 2, 3, 4, 5, 6, 7, 8, 9]).slice(0, -1)),
 (r([0, 1, 2, 3, 4, 5, 6, 7, 8, 9]).slice(2, -1)),
 (r([0, 1, 2, 3, 4, 5, 6, 7, 8, 9]).slice(0, 1)),
 (r([0, 1, 2, 3, 4, 5, 6, 7, 8, 9]).slice(-1, -1)),
 (r([0, 1, 2, 3, 4, 5, 6, 7, 8, 9]).slice(0, -2)),
 (r([0, 1, 2, 3, 4, 5, 6, 7, 8, 9]).slice(3, 4)),
 (r([0, 1, 2, 3, 4, 5, 6, 7, 8, 9]).nth(3)),
 (r([0, 1, 2, 3, 4, 5, 6, 7, 8, 9]).nth(-1)),
 (r({:a => 3, :b => 4})["a"]),
 (r({:a => 3, :b => 4})["b"]),
 (r({:a => 3, :b => 4})["c"]),
 (r("test").db_create),
 (r.db("test").table_create("tbl")),
 (r.db("test").table("tbl").delete),
 (r.db("test").table("tbl").insert(
   [{:num => 0, :name => "0", :id => 0},
    {:num => 1, :name => "1", :id => 1},
    {:num => 2, :name => "2", :id => 2},
    {:num => 3, :name => "3", :id => 3},
    {:num => 4, :name => "4", :id => 4},
    {:num => 5, :name => "5", :id => 5},
    {:num => 6, :name => "6", :id => 6},
    {:num => 7, :name => "7", :id => 7},
    {:num => 8, :name => "8", :id => 8},
    {:num => 9, :name => "9", :id => 9}]
 )),
 (r.db("test").table("tbl").orderby("id")),
 (r.db("test").table("tbl").update {|var_41| {:gb => var_41["id"].mod(3)}}),
 (r.db("test").table("tbl").groupby(["gb"], {:OUNT => true})
                                             ^^^^^^^^^^^^^^),
 (r.db("test").table("tbl").groupby(["gb"], {:SUM => "id"})),
 (r.db("test").table("tbl").groupby(["gb"], {:AVG => "id"})),
 (r("test").db_create),
 (r.db("test").table_create("tbl")),
 (r.db("test").table("tbl").delete),
 (r.db("test").table("tbl").insert(
   [{:num => 0, :name => "0", :id => 0},
    {:num => 1, :name => "1", :id => 1},
    {:num => 2, :name => "2", :id => 2},
    {:num => 3, :name => "3", :id => 3},
    {:num => 4, :name => "4", :id => 4},
    {:num => 5, :name => "5", :id => 5},
    {:num => 6, :name => "6", :id => 6},
    {:num => 7, :name => "7", :id => 7},
    {:num => 8, :name => "8", :id => 8},
    {:num => 9, :name => "9", :id => 9}]
 )),
 (r.db("test").table("tbl").orderby("id")),
 (r.db("test").table("tbl").coerce_to("array").orderby("id")),
 (r([{:id => 1}, {:id => 0}]).orderby("id")),
 (r(1).orderby("id")),
 (r([1]).nth(0).orderby("id")),
 (r([1]).orderby("id")),
 (r([{:num => 1}]).orderby("id")),
 (r([]).orderby("id")),
 (r.db("test").table("tbl").grouped_map_reduce(
   lambda {|var_46| var_46["id"].mod(4)},
   lambda {|var_47| var_47["id"]},
   {:base => 0},
   lambda {|var_48, var_49| var_48.add(var_49)}
 )),
 (r.db("test").table("tbl").grouped_map_reduce(
   lambda {|var_42| var_42["id"].mod(4)},
   lambda {|var_43| var_43["id"]},
   {:base => 0},
   lambda {|var_44, var_45| var_44.add(var_45)}
 )),
 (r.db("test").table("tbl").coerce_to("array").grouped_map_reduce(
   lambda {|var_50| var_50["id"].mod(4)},
   lambda {|var_51| var_51["id"]},
   {:base => 0},
   lambda {|var_52, var_53| var_52.add(var_53)}
 )),
 (r.db("test").table("tbl").grouped_map_reduce(
   lambda {|var_42| var_42["id"].mod(4)},
   lambda {|var_43| var_43["id"]},
   {:base => 0},
   lambda {|var_44, var_45| var_44.add(var_45)}
 )),
 (r([{:num => 0, :name => "0", :id => 0},
  {:num => 1, :name => "1", :id => 1},
  {:num => 2, :name => "2", :id => 2},
  {:num => 3, :name => "3", :id => 3},
  {:num => 4, :name => "4", :id => 4},
  {:num => 5, :name => "5", :id => 5},
  {:num => 6, :name => "6", :id => 6},
  {:num => 7, :name => "7", :id => 7},
  {:num => 8, :name => "8", :id => 8},
  {:num => 9, :name => "9", :id => 9}]).grouped_map_reduce(
   lambda {|var_54| var_54["id"].mod(4)},
   lambda {|var_55| var_55["id"]},
   {:base => 0},
   lambda {|var_56, var_57| var_56.add(var_57)}
 )),
 (r.db("test").table("tbl").grouped_map_reduce(
   lambda {|var_42| var_42["id"].mod(4)},
   lambda {|var_43| var_43["id"]},
   {:base => 0},
   lambda {|var_44, var_45| var_44.add(var_45)}
 )),
 (r([[{:num => 0, :name => "0", :id => 0},
   {:num => 1, :name => "1", :id => 1},
   {:num => 2, :name => "2", :id => 2},
   {:num => 3, :name => "3", :id => 3},
   {:num => 4, :name => "4", :id => 4},
   {:num => 5, :name => "5", :id => 5},
   {:num => 6, :name => "6", :id => 6},
   {:num => 7, :name => "7", :id => 7},
   {:num => 8, :name => "8", :id => 8},
   {:num => 9, :name => "9", :id => 9}]]).grouped_map_reduce(
   lambda {|var_58| var_58["id"].mod(4)},
   lambda {|var_59| var_59["id"]},
   {:base => 0},
   lambda {|var_60, var_61| var_60.add(var_61)}
 )),
 (r(1).grouped_map_reduce(
   lambda {|var_62| var_62["id"].mod(4)},
   lambda {|var_63| var_63["id"]},
   {:base => 0},
   lambda {|var_64, var_65| var_64.add(var_65)}
 )),
 (r.db("test").table("tbl").grouped_map_reduce(
   lambda {|var_42| var_42["id"].mod(4)},
   lambda {|var_43| var_43["id"]},
   {:base => 0},
   lambda {|var_44, var_45| var_44.add(var_45)}
 )),
 (r.db("test").table("tbl").orderby("id")),
 (r("test").db_create),
 (r.db("test").table_create("tbl")),
 (r.db("test").table("tbl").delete),
 (r.db("test").table("tbl").insert(
   [{:num => 0, :name => "0", :id => 0},
    {:num => 1, :name => "1", :id => 1},
    {:num => 2, :name => "2", :id => 2},
    {:num => 3, :name => "3", :id => 3},
    {:num => 4, :name => "4", :id => 4},
    {:num => 5, :name => "5", :id => 5},
    {:num => 6, :name => "6", :id => 6},
    {:num => 7, :name => "7", :id => 7},
    {:num => 8, :name => "8", :id => 8},
    {:num => 9, :name => "9", :id => 9}]
 )),
 (r(true).branch(3, 4)),
 (r(false).branch(4, 5)),
 (r(3).eq(3).branch("foo", "bar")),
 (r(5).branch(1, 2)),
 (r("test").db_create),
 (r.db("test").table_create("tbl")),
 (r.db("test").table("tbl").delete),
 (r.db("test").table("tbl").insert(
   [{:num => 0, :name => "0", :id => 0},
    {:num => 1, :name => "1", :id => 1},
    {:num => 2, :name => "2", :id => 2},
    {:num => 3, :name => "3", :id => 3},
    {:num => 4, :name => "4", :id => 4},
    {:num => 5, :name => "5", :id => 5},
    {:num => 6, :name => "6", :id => 6},
    {:num => 7, :name => "7", :id => 7},
    {:num => 8, :name => "8", :id => 8},
    {:num => 9, :name => "9", :id => 9}]
 )),
 (r.db("test").table("tbl").orderby("id")),
 (r.db("test").table("tbl").insert(
   [{:num => 0, :name => "0", :id => 0},
    {:num => 1, :name => "1", :id => 1},
    {:num => 2, :name => "2", :id => 2},
    {:num => 3, :name => "3", :id => 3},
    {:num => 4, :name => "4", :id => 4},
    {:num => 5, :name => "5", :id => 5},
    {:num => 6, :name => "6", :id => 6},
    {:num => 7, :name => "7", :id => 7},
    {:num => 8, :name => "8", :id => 8},
    {:num => 9, :name => "9", :id => 9},
    {:id => 10},
    {:id => 11},
    {:id => 12},
    {:id => 13},
    {:id => 14},
    {:id => 15},
    {:id => 16},
    {:id => 17},
    {:id => 18},
    {:id => 19}]
 )),
 (r.db("test").table("tbl").orderby("id")),
 (r.db("test").table("tbl").delete),
 (r.db("test").table("tbl").insert(
   [{:num => 0, :name => "0", :id => 0},
    {:num => 1, :name => "1", :id => 1},
    {:num => 2, :name => "2", :id => 2},
    {:num => 3, :name => "3", :id => 3},
    {:num => 4, :name => "4", :id => 4},
    {:num => 5, :name => "5", :id => 5},
    {:num => 6, :name => "6", :id => 6},
    {:num => 7, :name => "7", :id => 7},
    {:num => 8, :name => "8", :id => 8},
    {:num => 9, :name => "9", :id => 9}]
 )),
 (r.db("test").table("tbl").orderby("id")),
 (r("test").db_create),
 (r.db("test").table_create("tbl")),
 (r.db("test").table("tbl").delete),
 (r.db("test").table("tbl").insert(
   [{:num => 0, :name => "0", :id => 0},
    {:num => 1, :name => "1", :id => 1},
    {:num => 2, :name => "2", :id => 2},
    {:num => 3, :name => "3", :id => 3},
    {:num => 4, :name => "4", :id => 4},
    {:num => 5, :name => "5", :id => 5},
    {:num => 6, :name => "6", :id => 6},
    {:num => 7, :name => "7", :id => 7},
    {:num => 8, :name => "8", :id => 8},
    {:num => 9, :name => "9", :id => 9}]
 )),
 (r("14044001728944632967").db_create),
 (r.db("14044001728944632967").table_create("17887501570060339481")),
 (r.db("14044001728944632967").table_create("17887501570060339482")),
 (r.db("14044001728944632967").table("17887501570060339481").insert(
   [{:a => 0, :id => 0},
    {:a => 2, :id => 1},
    {:a => 4, :id => 2},
    {:a => 6, :id => 3},
    {:a => 8, :id => 4},
    {:a => 10, :id => 5},
    {:a => 12, :id => 6},
    {:a => 14, :id => 7},
    {:a => 16, :id => 8},
    {:a => 18, :id => 9}]
 )),
 (r.db("14044001728944632967").table("17887501570060339482").insert(
   [{:b => 0, :id => 0},
    {:b => 100, :id => 1},
    {:b => 200, :id => 2},
    {:b => 300, :id => 3},
    {:b => 400, :id => 4},
    {:b => 500, :id => 5},
    {:b => 600, :id => 6},
    {:b => 700, :id => 7},
    {:b => 800, :id => 8},
    {:b => 900, :id => 9}]
 )),
 (r.db("14044001728944632967").table("17887501570060339481").eq_join(
   "a",
   r.db("14044001728944632967").table("17887501570060339482")
 )),
 (r.db("14044001728944632967").table("17887501570060339481").inner_join(
   r.db("14044001728944632967").table("17887501570060339482")
 ) {|var_66, var_67| var_66["a"].eq(var_67["id"])}),
 (r.db("14044001728944632967").table("17887501570060339481").eq_join(
   "a",
   r.db("14044001728944632967").table("17887501570060339482")
 )),
 (r.db("14044001728944632967").table("17887501570060339481").eq_join(
   "a",
   r.db("14044001728944632967").table("17887501570060339482")
 )),
 (r.db("14044001728944632967").table("17887501570060339481").filter {|var_68|
   r.db("14044001728944632967").table("17887501570060339482").get(
     var_68["a"]
   ).eq(nil)
 }.map {|var_69| {:left => var_69}}),
 (r.db("14044001728944632967").table("17887501570060339481").outer_join(
   r.db("14044001728944632967").table("17887501570060339482")
 ) {|var_70, var_71| var_70["a"].eq(var_71["id"])}),
 (r.db("14044001728944632967").table("17887501570060339481").outer_join(
   r.db("14044001728944632967").table("17887501570060339482")
 ) {|var_72, var_73| var_72["a"].eq(var_73["id"])}.zip),
 (r("14044001728944632967").db_drop),
 (r("test").db_create),
 (r.db("test").table_create("tbl")),
 (r.db("test").table("tbl").delete),
 (r.db("test").table("tbl").insert(
   [{:num => 0, :name => "0", :id => 0},
    {:num => 1, :name => "1", :id => 1},
    {:num => 2, :name => "2", :id => 2},
    {:num => 3, :name => "3", :id => 3},
    {:num => 4, :name => "4", :id => 4},
    {:num => 5, :name => "5", :id => 5},
    {:num => 6, :name => "6", :id => 6},
    {:num => 7, :name => "7", :id => 7},
    {:num => 8, :name => "8", :id => 8},
    {:num => 9, :name => "9", :id => 9}]
 )),
 (r(false).any),
 (r(true).any(false)),
 (r(false).any(true)),
 (r(false).any(false, true)),
 (r(false).all),
 (r(true).all(false)),
 (r(true).all(true)),
 (r(true).all(true, true)),
 (r(true).all(3)),
 (r(4).any(true)),
 (r("test").db_create),
 (r.db("test").table_create("tbl")),
 (r.db("test").table("tbl").delete),
 (r.db("test").table("tbl").insert(
   [{:num => 0, :name => "0", :id => 0},
    {:num => 1, :name => "1", :id => 1},
    {:num => 2, :name => "2", :id => 2},
    {:num => 3, :name => "3", :id => 3},
    {:num => 4, :name => "4", :id => 4},
    {:num => 5, :name => "5", :id => 5},
    {:num => 6, :name => "6", :id => 6},
    {:num => 7, :name => "7", :id => 7},
    {:num => 8, :name => "8", :id => 8},
    {:num => 9, :name => "9", :id => 9}]
 )),
 (r.db("test").table("tbl").orderby("id")),
 (r.db("test").table("tbl").insert([[], {}, {:a => 1}, {:a => 2, :id => -1}])),
 (r.db("test").table("tbl").get("0837d29d-a34e-43eb-a5e1-9595569951d2").delete),
 (r.db("test").table("tbl").filter {|var_74|
   var_74["id"].lt(0).any(var_74["id"].gt(1000))
 }.delete),
 (r.db("test").table("tbl").orderby("id")),
 (r("test").db_create),
 (r.db("test").table_create("tbl")),
 (r.db("test").table("tbl").delete),
 (r.db("test").table("tbl").insert(
   [{:num => 0, :name => "0", :id => 0},
    {:num => 1, :name => "1", :id => 1},
    {:num => 2, :name => "2", :id => 2},
    {:num => 3, :name => "3", :id => 3},
    {:num => 4, :name => "4", :id => 4},
    {:num => 5, :name => "5", :id => 5},
    {:num => 6, :name => "6", :id => 6},
    {:num => 7, :name => "7", :id => 7},
    {:num => 8, :name => "8", :id => 8},
    {:num => 9, :name => "9", :id => 9}]
 )),
 (r([{:id => 1}, {:id => 2}]).map {|var_75| var_75["id"]}),
 (r(1).map { nil}),
 (r.db("test").table("tbl").filter({:num => 1})),
 (r.db("test").table("tbl").orderby("id").map {|var_76|
   r.db("test").table("tbl").filter {|var_77|
     var_77["id"].lt(var_76["id"])
   }.coerce_to("array")
 }),
 (r("test").db_create),
 (r.db("test").table_create("tbl")),
 (r.db("test").table("tbl").delete),
 (r.db("test").table("tbl").insert(
   [{:num => 0, :name => "0", :id => 0},
    {:num => 1, :name => "1", :id => 1},
    {:num => 2, :name => "2", :id => 2},
    {:num => 3, :name => "3", :id => 3},
    {:num => 4, :name => "4", :id => 4},
    {:num => 5, :name => "5", :id => 5},
    {:num => 6, :name => "6", :id => 6},
    {:num => 7, :name => "7", :id => 7},
    {:num => 8, :name => "8", :id => 8},
    {:num => 9, :name => "9", :id => 9}]
 )),
 (r({:a => 1}).merge({:b => 2})),
 (r({:a => 1}).merge({:a => 2})),
 (r("test").db_create),
 (r.db("test").table_create("tbl")),
 (r.db("test").table("tbl").delete),
 (r.db("test").table("tbl").insert(
   [{:num => 0, :name => "0", :id => 0},
    {:num => 1, :name => "1", :id => 1},
    {:num => 2, :name => "2", :id => 2},
    {:num => 3, :name => "3", :id => 3},
    {:num => 4, :name => "4", :id => 4},
    {:num => 5, :name => "5", :id => 5},
    {:num => 6, :name => "6", :id => 6},
    {:num => 7, :name => "7", :id => 7},
    {:num => 8, :name => "8", :id => 8},
    {:num => 9, :name => "9", :id => 9}]
 )),
 (r.db("test").table("tbl").replace { 1}),
 (r.db("test").table("tbl").orderby("id")),
 (r.db("test").table("tbl").get(0).replace { nil}),
 (r.db("test").table("tbl").orderby("id")),
 (r.db("test").table("tbl").get(0).replace { nil}),
 (r.db("test").table("tbl").orderby("id")),
 (r.db("test").table("tbl").get(0).replace { {:num => 0, :name => "0", :id => 0}}),
 (r.db("test").table("tbl").get(-1).replace { {:id => []}}),
 (r.db("test").table("tbl").orderby("id")),
 (r.db("test").table("tbl").get(0).replace {|var_78|
   var_78.eq(nil).branch(
     {:num => 0, :name => "0", :id => 0},
     {:num => 1, :name => "1", :id => 1}
   )
 }),
 (r.db("test").table("tbl").get(0).replace {|var_79|
   var_79.eq(nil).branch(
     {:num => 1, :name => "1", :id => 1},
     {:num => 0, :name => "0", :id => 0}
   )
 }),
 (r.db("test").table("tbl").orderby("id")),
 (r("test").db_create),
 (r.db("test").table_create("tbl")),
 (r.db("test").table("tbl").delete),
 (r.db("test").table("tbl").insert(
   [{:num => 0, :name => "0", :id => 0},
    {:num => 1, :name => "1", :id => 1},
    {:num => 2, :name => "2", :id => 2},
    {:num => 3, :name => "3", :id => 3},
    {:num => 4, :name => "4", :id => 4},
    {:num => 5, :name => "5", :id => 5},
    {:num => 6, :name => "6", :id => 6},
    {:num => 7, :name => "7", :id => 7},
    {:num => 8, :name => "8", :id => 8},
    {:num => 9, :name => "9", :id => 9}]
 )),
 (r(true).not),
 (r(false).not),
 (r(3).not),
 (r("test").db_create),
 (r.db("test").table_create("tbl")),
 (r.db("test").table("tbl").delete),
 (r.db("test").table("tbl").insert(
   [{:num => 0, :name => "0", :id => 0},
    {:num => 1, :name => "1", :id => 1},
    {:num => 2, :name => "2", :id => 2},
    {:num => 3, :name => "3", :id => 3},
    {:num => 4, :name => "4", :id => 4},
    {:num => 5, :name => "5", :id => 5},
    {:num => 6, :name => "6", :id => 6},
    {:num => 7, :name => "7", :id => 7},
    {:num => 8, :name => "8", :id => 8},
    {:num => 9, :name => "9", :id => 9}]
 )),
 (r.db("test").table("tbl").orderby("id").nth(2)),
 (r.db("test").table("tbl").orderby("id").nth(-1)),
 (r.db("test").table("tbl").nth(-1)),
 (r("test").db_create),
 (r.db("test").table_create("tbl")),
 (r.db("test").table("tbl").delete),
 (r.db("test").table("tbl").insert(
   [{:num => 0, :name => "0", :id => 0},
    {:num => 1, :name => "1", :id => 1},
    {:num => 2, :name => "2", :id => 2},
    {:num => 3, :name => "3", :id => 3},
    {:num => 4, :name => "4", :id => 4},
    {:num => 5, :name => "5", :id => 5},
    {:num => 6, :name => "6", :id => 6},
    {:num => 7, :name => "7", :id => 7},
    {:num => 8, :name => "8", :id => 8},
    {:num => 9, :name => "9", :id => 9}]
 )),
 (r(1).div(0)),
 (r(0).div(0)),
 (r(1.0e+200).mul(1.0e+300)),
 (r(1.0e+100).mul(1.0e+100)),
 (r("test").db_create),
 (r.db("test").table_create("tbl")),
 (r.db("test").table("tbl").delete),
 (r.db("test").table("tbl").insert(
   [{:num => 0, :name => "0", :id => 0},
    {:num => 1, :name => "1", :id => 1},
    {:num => 2, :name => "2", :id => 2},
    {:num => 3, :name => "3", :id => 3},
    {:num => 4, :name => "4", :id => 4},
    {:num => 5, :name => "5", :id => 5},
    {:num => 6, :name => "6", :id => 6},
    {:num => 7, :name => "7", :id => 7},
    {:num => 8, :name => "8", :id => 8},
    {:num => 9, :name => "9", :id => 9}]
 )),
 (r.db("a").table_create("b", {:bad_opt => true})),
 (r.db("a").table("b", {:bad_opt => true})),
 (r.db("a")),
 (r("test").db_create),
 (r.db("test").table_create("tbl")),
 (r.db("test").table("tbl").delete),
 (r.db("test").table("tbl").insert(
   [{:num => 0, :name => "0", :id => 0},
    {:num => 1, :name => "1", :id => 1},
    {:num => 2, :name => "2", :id => 2},
    {:num => 3, :name => "3", :id => 3},
    {:num => 4, :name => "4", :id => 4},
    {:num => 5, :name => "5", :id => 5},
    {:num => 6, :name => "6", :id => 6},
    {:num => 7, :name => "7", :id => 7},
    {:num => 8, :name => "8", :id => 8},
    {:num => 9, :name => "9", :id => 9}]
 )),
 (r(5).add(3)),
 (r(5).add(3)),
 (r(5).add(3)),
 (r(2).add(3, 3)),
 (r(5).sub(3)),
 (r(5).sub(3)),
 (r(5).sub(3)),
 (r(5).mod(3)),
 (r(5).mod(3)),
 (r(5).mod(3)),
 (r(5).mul(3)),
 (r(5).mul(3)),
 (r(5).mul(3)),
 (r(15).div(3)),
 (r(15).div(3)),
 (r(15).div(3)),
 (r(3).lt(2)),
 (r(3).lt(3)),
 (r(3).lt(4)),
 (r(3).lt(2)),
 (r(3).lt(3)),
 (r(3).lt(4)),
 (r(3).lt(2)),
 (r(3).lt(3)),
 (r(3).lt(4)),
 (r(3).le(2)),
 (r(3).le(3)),
 (r(3).le(4)),
 (r(3).le(2)),
 (r(3).le(3)),
 (r(3).le(4)),
 (r(3).le(2)),
 (r(3).le(3)),
 (r(3).le(4)),
 (r(3).gt(2)),
 (r(3).gt(3)),
 (r(3).gt(4)),
 (r(3).gt(2)),
 (r(3).gt(3)),
 (r(3).gt(4)),
 (r(3).gt(2)),
 (r(3).gt(3)),
 (r(3).gt(4)),
 (r(3).ge(2)),
 (r(3).ge(3)),
 (r(3).ge(4)),
 (r(3).ge(2)),
 (r(3).ge(3)),
 (r(3).ge(4)),
 (r(3).ge(2)),
 (r(3).ge(3)),
 (r(3).ge(4)),
 (r(3).eq(2)),
 (r(3).eq(3)),
 (r(3).ne(2)),
 (r(3).ne(3)),
 (r(true).all(true, true)),
 (r(true).all(false, true)),
 (r(true).all(true, true)),
 (r(true).all(false, true)),
 (r(true).all(true)),
 (r(true).all(false)),
 (r(false).any(false, false)),
 (r(false).any(true, false)),
 (r(false).any(false, false)),
 (r(false).any(true, false)),
 (r(false).any(false)),
 (r(true).any(false)),
 (r("test").db_create),
 (r.db("test").table_create("tbl")),
 (r.db("test").table("tbl").delete),
 (r.db("test").table("tbl").insert(
   [{:num => 0, :name => "0", :id => 0},
    {:num => 1, :name => "1", :id => 1},
    {:num => 2, :name => "2", :id => 2},
    {:num => 3, :name => "3", :id => 3},
    {:num => 4, :name => "4", :id => 4},
    {:num => 5, :name => "5", :id => 5},
    {:num => 6, :name => "6", :id => 6},
    {:num => 7, :name => "7", :id => 7},
    {:num => 8, :name => "8", :id => 8},
    {:num => 9, :name => "9", :id => 9}]
 )),
 (r.db("test").table("tbl").orderby("id")),
 (r.db("test").table("tbl").orderby(r("id").asc)),
 (r.db("test").table("tbl").orderby("id")),
 (r.db("test").table("tbl").orderby(r("id").asc)),
 (r.db("test").table("tbl").orderby(r("id").desc)),
 (r.db("test").table("tbl").map {|var_80|
   {:num => var_80["id"].mod(2), :id => var_80["id"]}
 }.orderby("num", r("id").desc)),
 (r("test").db_create),
 (r.db("test").table_create("tbl")),
 (r.db("test").table("tbl").delete),
 (r.db("test").table("tbl").insert(
   [{:num => 0, :name => "0", :id => 0},
    {:num => 1, :name => "1", :id => 1},
    {:num => 2, :name => "2", :id => 2},
    {:num => 3, :name => "3", :id => 3},
    {:num => 4, :name => "4", :id => 4},
    {:num => 5, :name => "5", :id => 5},
    {:num => 6, :name => "6", :id => 6},
    {:num => 7, :name => "7", :id => 7},
    {:num => 8, :name => "8", :id => 8},
    {:num => 9, :name => "9", :id => 9}]
 )),
 (r([{:b => 0, :a => 0, :id => 100},
  {:b => 1, :a => 1, :id => 101},
  {:b => 2, :a => 2, :id => 102},
  {:b => 0, :a => 3, :id => 103},
  {:b => 1, :a => 4, :id => 104},
  {:b => 2, :a => 5, :id => 105},
  {:b => 0, :a => 6, :id => 106},
  {:b => 1, :a => 7, :id => 107},
  {:b => 2, :a => 8, :id => 108},
  {:b => 0, :a => 9, :id => 109}]).orderby("s")),
 (r([{:b => 0, :a => 0, :id => 100},
  {:b => 1, :a => 1, :id => 101},
  {:b => 2, :a => 2, :id => 102},
  {:b => 0, :a => 3, :id => 103},
  {:b => 1, :a => 4, :id => 104},
  {:b => 2, :a => 5, :id => 105},
  {:b => 0, :a => 6, :id => 106},
  {:b => 1, :a => 7, :id => 107},
  {:b => 2, :a => 8, :id => 108},
  {:b => 0, :a => 9, :id => 109}]).orderby(r("a").desc)),
 (r([{:b => 0, :a => 0, :id => 100},
  {:b => 1, :a => 1, :id => 101},
  {:b => 2, :a => 2, :id => 102},
  {:b => 0, :a => 3, :id => 103},
  {:b => 1, :a => 4, :id => 104},
  {:b => 2, :a => 5, :id => 105},
  {:b => 0, :a => 6, :id => 106},
  {:b => 1, :a => 7, :id => 107},
  {:b => 2, :a => 8, :id => 108},
  {:b => 0, :a => 9, :id => 109}]).filter {|var_81| var_81["b"].eq(0)}.orderby(
   "+a"
 )),
 (r([{:b => 0, :a => 0, :id => 100},
  {:b => 1, :a => 1, :id => 101},
  {:b => 2, :a => 2, :id => 102},
  {:b => 0, :a => 3, :id => 103},
  {:b => 1, :a => 4, :id => 104},
  {:b => 2, :a => 5, :id => 105},
  {:b => 0, :a => 6, :id => 106},
  {:b => 1, :a => 7, :id => 107},
  {:b => 2, :a => 8, :id => 108},
  {:b => 0, :a => 9, :id => 109}]).filter {|var_82| var_82["b"].eq(0)}.orderby(
   r("a").desc
 )),
 (r("test").db_create),
 (r.db("test").table_create("tbl")),
 (r.db("test").table("tbl").delete),
 (r.db("test").table("tbl").insert(
   [{:num => 0, :name => "0", :id => 0},
    {:num => 1, :name => "1", :id => 1},
    {:num => 2, :name => "2", :id => 2},
    {:num => 3, :name => "3", :id => 3},
    {:num => 4, :name => "4", :id => 4},
    {:num => 5, :name => "5", :id => 5},
    {:num => 6, :name => "6", :id => 6},
    {:num => 7, :name => "7", :id => 7},
    {:num => 8, :name => "8", :id => 8},
    {:num => 9, :name => "9", :id => 9}]
 )),
 (r.db("test").table("tbl", {:use_outdated => true}).insert(
   {:id => 0},
   {:upsert => true}
 )),
 (r.db("test").table("tbl", {:use_outdated => true}).filter {|var_83|
   var_83["id"].lt(5)
 }.update { {}}),
 (r.db("test").table("tbl", {:use_outdated => true}).update { {}}),
 (r.db("test").table("tbl", {:use_outdated => true}).filter {|var_84|
   var_84["id"].lt(5)
 }.delete),
 (r.db("test").table("tbl", {:use_outdated => true}).filter {|var_85|
   var_85["id"].lt(5)
 }.between({:left_bound => 2, :right_bound => 3}).replace { nil}),
 (r("test").db_create),
 (r.db("test").table_create("tbl")),
 (r.db("test").table("tbl").delete),
 (r.db("test").table("tbl").insert(
   [{:num => 0, :name => "0", :id => 0},
    {:num => 1, :name => "1", :id => 1},
    {:num => 2, :name => "2", :id => 2},
    {:num => 3, :name => "3", :id => 3},
    {:num => 4, :name => "4", :id => 4},
    {:num => 5, :name => "5", :id => 5},
    {:num => 6, :name => "6", :id => 6},
    {:num => 7, :name => "7", :id => 7},
    {:num => 8, :name => "8", :id => 8},
    {:num => 9, :name => "9", :id => 9}]
 )),
 (r.db("test").table("tbl").map {|var_86| var_86.pluck("id", "name")}.union(
   r.db("test").table("tbl").map {|var_87| var_87.pluck("id", "num")}
 )),
 (r.db("test").table("tbl").map {|var_88| var_88.pluck("id", "name")}.union(
   r.db("test").table("tbl").map {|var_89| var_89.pluck("id", "num")}
 )),
 (r.db("test").table("tbl").map {|var_88| var_88.pluck("id", "name")}.union(
   r.db("test").table("tbl").map {|var_89| var_89.pluck("id", "num")}
 ).count),
 (r("test").db_create),
 (r.db("test").table_create("tbl")),
 (r.db("test").table("tbl").delete),
 (r.db("test").table("tbl").insert(
   [{:num => 0, :name => "0", :id => 0},
    {:num => 1, :name => "1", :id => 1},
    {:num => 2, :name => "2", :id => 2},
    {:num => 3, :name => "3", :id => 3},
    {:num => 4, :name => "4", :id => 4},
    {:num => 5, :name => "5", :id => 5},
    {:num => 6, :name => "6", :id => 6},
    {:num => 7, :name => "7", :id => 7},
    {:num => 8, :name => "8", :id => 8},
    {:num => 9, :name => "9", :id => 9}]
 )),
 (r([{:c => 3, :b => 2, :a => 1}, {:c => 6, :b => 5, :a => 4}]).pluck),
 (r([{:c => 3, :b => 2, :a => 1}, {:c => 6, :b => 5, :a => 4}]).pluck("a")),
 (r([{:c => 3, :b => 2, :a => 1}, {:c => 6, :b => 5, :a => 4}]).pluck("a", "b")),
 (r("test").db_create),
 (r.db("test").table_create("tbl")),
 (r.db("test").table("tbl").delete),
 (r.db("test").table("tbl").insert(
   [{:num => 0, :name => "0", :id => 0},
    {:num => 1, :name => "1", :id => 1},
    {:num => 2, :name => "2", :id => 2},
    {:num => 3, :name => "3", :id => 3},
    {:num => 4, :name => "4", :id => 4},
    {:num => 5, :name => "5", :id => 5},
    {:num => 6, :name => "6", :id => 6},
    {:num => 7, :name => "7", :id => 7},
    {:num => 8, :name => "8", :id => 8},
    {:num => 9, :name => "9", :id => 9}]
 )),
 (r.db("test").table("tbl").orderby("id")),
 (r.db("test").table("tbl").get(0).delete),
 (r.db("test").table("tbl").get(0).delete),
 (r.db("test").table("tbl").orderby("id")),
 (r.db("test").table("tbl").insert({:num => 0, :name => "0", :id => 0})),
 (r.db("test").table("tbl").orderby("id")),
 (r("test").db_create),
 (r.db("test").table_create("tbl")),
 (r.db("test").table("tbl").delete),
 (r.db("test").table("tbl").insert(
   [{:num => 0, :name => "0", :id => 0},
    {:num => 1, :name => "1", :id => 1},
    {:num => 2, :name => "2", :id => 2},
    {:num => 3, :name => "3", :id => 3},
    {:num => 4, :name => "4", :id => 4},
    {:num => 5, :name => "5", :id => 5},
    {:num => 6, :name => "6", :id => 6},
    {:num => 7, :name => "7", :id => 7},
    {:num => 8, :name => "8", :id => 8},
    {:num => 9, :name => "9", :id => 9}]
 )),
 (r.db("test").table("tbl").insert(true)),
 (r.db("test").table("tbl").insert([true, true])),
 (r("test").db_create),
 (r.db("test").table_create("tbl")),
 (r.db("test").table("tbl").delete),
 (r.db("test").table("tbl").insert(
   [{:num => 0, :name => "0", :id => 0},
    {:num => 1, :name => "1", :id => 1},
    {:num => 2, :name => "2", :id => 2},
    {:num => 3, :name => "3", :id => 3},
    {:num => 4, :name => "4", :id => 4},
    {:num => 5, :name => "5", :id => 5},
    {:num => 6, :name => "6", :id => 6},
    {:num => 7, :name => "7", :id => 7},
    {:num => 8, :name => "8", :id => 8},
    {:num => 9, :name => "9", :id => 9}]
 )),
 (r.db("test").table("tbl").between({:left_bound => 1, :right_bound => 3})),
 (r.db("test").table("tbl").between({:left_bound => 3, :right_bound => 3})),
 (r.db("test").table("tbl").between({:left_bound => 2})),
 (r.db("test").table("tbl").between({:left_bound => 1, :right_bound => 3})),
 (r.db("test").table("tbl").between({:right_bound => 4})),
 (r([1]).between({:left_bound => 1, :right_bound => 3})),
 (r([1, 2]).between({:left_bound => 1, :right_bound => 3})),
 (r("test").db_create),
 (r.db("test").table_create("tbl")),
 (r.db("test").table("tbl").delete),
 (r.db("test").table("tbl").insert(
   [{:num => 0, :name => "0", :id => 0},
    {:num => 1, :name => "1", :id => 1},
    {:num => 2, :name => "2", :id => 2},
    {:num => 3, :name => "3", :id => 3},
    {:num => 4, :name => "4", :id => 4},
    {:num => 5, :name => "5", :id => 5},
    {:num => 6, :name => "6", :id => 6},
    {:num => 7, :name => "7", :id => 7},
    {:num => 8, :name => "8", :id => 8},
    {:num => 9, :name => "9", :id => 9}]
 )),
 (r([1, 2, 3]).reduce({:base => 0}) {|var_90, var_91| var_90.add(var_91)}),
 (r(1).reduce({:base => 0}) { 0}),
 (r.db("test").table("tbl").map {|var_92| var_92.contains("id")}),
 (r.db("test").table("tbl").map {|var_93| var_93.contains("id").not}),
 (r.db("test").table("tbl").map {|var_94| var_94.contains("id").not}),
 (r.db("test").table("tbl").map {|var_95| var_95.contains("id").not}),
 (r.db("test").table("tbl").map {|var_96| var_96.contains("id")}),
 (r.db("test").table("tbl").map {|var_97| var_97.contains("id").not}),
 (r("test").db_create),
 (r.db("test").table_create("tbl")),
 (r.db("test").table("tbl").delete),
 (r.db("test").table("tbl").insert(
   [{:num => 0, :name => "0", :id => 0},
    {:num => 1, :name => "1", :id => 1},
    {:num => 2, :name => "2", :id => 2},
    {:num => 3, :name => "3", :id => 3},
    {:num => 4, :name => "4", :id => 4},
    {:num => 5, :name => "5", :id => 5},
    {:num => 6, :name => "6", :id => 6},
    {:num => 7, :name => "7", :id => 7},
    {:num => 8, :name => "8", :id => 8},
    {:num => 9, :name => "9", :id => 9}]
 )),
 (lambda {|var_98| [var_98, var_98]}.funcall(1)),
 (r("test").db_create),
 (r.db("test").table_create("tbl")),
 (r.db("test").table("tbl").delete),
 (r.db("test").table("tbl").insert(
   [{:num => 0, :name => "0", :id => 0},
    {:num => 1, :name => "1", :id => 1},
    {:num => 2, :name => "2", :id => 2},
    {:num => 3, :name => "3", :id => 3},
    {:num => 4, :name => "4", :id => 4},
    {:num => 5, :name => "5", :id => 5},
    {:num => 6, :name => "6", :id => 6},
    {:num => 7, :name => "7", :id => 7},
    {:num => 8, :name => "8", :id => 8},
    {:num => 9, :name => "9", :id => 9}]
 )),
 (r([0, 1, 2, 3, 4, 5]).nth(1)),
 (r([0, 1, 2, 3, 4, 5]).slice(2, -1)),
 (r([0, 1, 2, 3, 4, 5]).slice(2, 5)),
 (r([0, 1, 2, 3, 4, 5]).slice(2, 4)),
 (r([0, 1, 2, 3, 4, 5]).slice(2, 4)),
 (r("test").db_create),
 (r.db("test").table_create("tbl")),
 (r.db("test").table("tbl").delete),
 (r.db("test").table("tbl").insert(
   [{:num => 0, :name => "0", :id => 0},
    {:num => 1, :name => "1", :id => 1},
    {:num => 2, :name => "2", :id => 2},
    {:num => 3, :name => "3", :id => 3},
    {:num => 4, :name => "4", :id => 4},
    {:num => 5, :name => "5", :id => 5},
    {:num => 6, :name => "6", :id => 6},
    {:num => 7, :name => "7", :id => 7},
    {:num => 8, :name => "8", :id => 8},
    {:num => 9, :name => "9", :id => 9}]
 )),
 (r([]).limit(0)),
 (r([1, 2]).limit(0)),
 (r([1, 2]).limit(1)),
 (r([1, 2]).limit(5)),
 (r([]).limit(-1)),
 (r([]).skip(0)),
 (r([1, 2]).skip(5)),
 (r([1, 2]).skip(0)),
 (r([1, 2]).skip(1)),
 (r([]).distinct),
 (r([1,
  2,
  3,
  1,
  2,
  3,
  1,
  2,
  3,
  1,
  2,
  3,
  1,
  2,
  3,
  1,
  2,
  3,
  1,
  2,
  3,
  1,
  2,
  3,
  1,
  2,
  3,
  1,
  2,
  3]).distinct),
 (r([1, 2, 3, 2]).distinct),
 (r([true, 2, false, 2]).distinct),
 (r("test").db_create),
 (r.db("test").table_create("tbl")),
 (r.db("test").table("tbl").delete),
 (r.db("test").table("tbl").insert(
   [{:num => 0, :name => "0", :id => 0},
    {:num => 1, :name => "1", :id => 1},
    {:num => 2, :name => "2", :id => 2},
    {:num => 3, :name => "3", :id => 3},
    {:num => 4, :name => "4", :id => 4},
    {:num => 5, :name => "5", :id => 5},
    {:num => 6, :name => "6", :id => 6},
    {:num => 7, :name => "7", :id => 7},
    {:num => 8, :name => "8", :id => 8},
    {:num => 9, :name => "9", :id => 9}]
 )),
 (r.db("test").table("tbl").insert(
   {:id =>
      "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"}
 )),
 (r("test").db_create),
 (r.db("test").table_create("tbl")),
 (r.db("test").table("tbl").delete),
 (r.db("test").table("tbl").insert(
   [{:num => 0, :name => "0", :id => 0},
    {:num => 1, :name => "1", :id => 1},
    {:num => 2, :name => "2", :id => 2},
    {:num => 3, :name => "3", :id => 3},
    {:num => 4, :name => "4", :id => 4},
    {:num => 5, :name => "5", :id => 5},
    {:num => 6, :name => "6", :id => 6},
    {:num => 7, :name => "7", :id => 7},
    {:num => 8, :name => "8", :id => 8},
    {:num => 9, :name => "9", :id => 9}]
 )),
 (r([]).typeof),
 (r(true).typeof),
 (r(nil).typeof),
 (r(1).typeof),
 (r({}).typeof),
 (r("").typeof),
 (r.db("test").typeof),
 (r.db("test").table("tbl").typeof),
 (r.db("test").table("tbl").map {|var_99| var_99}.typeof),
 (r.db("test").table("tbl").filter {|var_100| true}.typeof),
 (r.db("test").table("tbl").get(0).typeof),
 (lambda { nil}.typeof),
 (r("test").db_create),
 (r.db("test").table_create("tbl")),
 (r.db("test").table("tbl").delete),
 (r.db("test").table("tbl").insert(
   [{:num => 0, :name => "0", :id => 0},
    {:num => 1, :name => "1", :id => 1},
    {:num => 2, :name => "2", :id => 2},
    {:num => 3, :name => "3", :id => 3},
    {:num => 4, :name => "4", :id => 4},
    {:num => 5, :name => "5", :id => 5},
    {:num => 6, :name => "6", :id => 6},
    {:num => 7, :name => "7", :id => 7},
    {:num => 8, :name => "8", :id => 8},
    {:num => 9, :name => "9", :id => 9}]
 )),
 (r.db("test").table("tbl").orderby("id")),
 (r.db("test").table("tbl").get(0).update { nil}),
 (r.db("test").table("tbl").get(11).update { nil}),
 (r.db("test").table("tbl").get(11).update { {}}),
 (r.db("test").table("tbl").orderby("id")),
 (r("test").db_create),
 (r.db("test").table_create("tbl")),
 (r.db("test").table("tbl").delete),
 (r.db("test").table("tbl").insert(
   [{:num => 0, :name => "0", :id => 0},
    {:num => 1, :name => "1", :id => 1},
    {:num => 2, :name => "2", :id => 2},
    {:num => 3, :name => "3", :id => 3},
    {:num => 4, :name => "4", :id => 4},
    {:num => 5, :name => "5", :id => 5},
    {:num => 6, :name => "6", :id => 6},
    {:num => 7, :name => "7", :id => 7},
    {:num => 8, :name => "8", :id => 8},
    {:num => 9, :name => "9", :id => 9}]
 )),
 (r({:a => 1, :b => 2, :c => 3}).without("a", "c")),
 (r({:a => 1}).without("b")),
 (r({}).without("b")),
 (r("test").db_create),
 (r.db("test").table_create("tbl")),
 (r.db("test").table("tbl").delete),
 (r.db("test").table("tbl").insert(
   [{:num => 0, :name => "0", :id => 0},
    {:num => 1, :name => "1", :id => 1},
    {:num => 2, :name => "2", :id => 2},
    {:num => 3, :name => "3", :id => 3},
    {:num => 4, :name => "4", :id => 4},
    {:num => 5, :name => "5", :id => 5},
    {:num => 6, :name => "6", :id => 6},
    {:num => 7, :name => "7", :id => 7},
    {:num => 8, :name => "8", :id => 8},
    {:num => 9, :name => "9", :id => 9}]
 )),
 (r.db_list),
 (r("6570430476421813371").db_create),
 (r("6570430476421813371").db_create),
 (r.db_list),
 (r.db("6570430476421813371").table_create("7953218286624157490")),
 (r.db("6570430476421813371").table_create("7953218286624157490")),
 (r.db("6570430476421813371").table_list),
 (r.db("6570430476421813371").table("7953218286624157490").insert({:id => 0})),
 (r.db("6570430476421813371").table("7953218286624157490")),
 (r.db("6570430476421813371").table_create(
   "79532182866241574907953218286624157490"
 )),
 (r.db("6570430476421813371").table_list),
 (r.db("").table("")),
 (r.db("test").table("7953218286624157490")),
 (r.db("6570430476421813371").table("")),
 (r.db("6570430476421813371").table_drop(
   "79532182866241574907953218286624157490"
 )),
 (r.db("6570430476421813371").table("79532182866241574907953218286624157490")),
 (r("6570430476421813371").db_drop),
 (r.db_list),
 (r.db("6570430476421813371").table("7953218286624157490")),
 (r("6570430476421813371").db_create),
 (r.db("6570430476421813371").table("7953218286624157490")),
 (r.db("6570430476421813371").table_create("7953218286624157490")),
 (r.db("6570430476421813371").table("7953218286624157490")),
 (r.db("6570430476421813371").table("7953218286624157490").insert(
   [{:num => 0, :name => "0", :id => 0},
    {:num => 1, :name => "1", :id => 1},
    {:num => 2, :name => "2", :id => 2},
    {:num => 3, :name => "3", :id => 3},
    {:num => 4, :name => "4", :id => 4},
    {:num => 5, :name => "5", :id => 5},
    {:num => 6, :name => "6", :id => 6},
    {:num => 7, :name => "7", :id => 7},
    {:num => 8, :name => "8", :id => 8},
    {:num => 9, :name => "9", :id => 9}],
   {:upsert => true}
 )),
 (r.db("6570430476421813371").table("7953218286624157490").insert(
   [{:num => 0, :name => "0", :id => 0},
    {:num => 1, :name => "1", :id => 1},
    {:num => 2, :name => "2", :id => 2},
    {:num => 3, :name => "3", :id => 3},
    {:num => 4, :name => "4", :id => 4},
    {:num => 5, :name => "5", :id => 5},
    {:num => 6, :name => "6", :id => 6},
    {:num => 7, :name => "7", :id => 7},
    {:num => 8, :name => "8", :id => 8},
    {:num => 9, :name => "9", :id => 9},
    {:num => 0, :name => "0", :id => 0},
    {:num => 1, :name => "1", :id => 1},
    {:num => 2, :name => "2", :id => 2},
    {:num => 3, :name => "3", :id => 3},
    {:num => 4, :name => "4", :id => 4},
    {:num => 5, :name => "5", :id => 5},
    {:num => 6, :name => "6", :id => 6},
    {:num => 7, :name => "7", :id => 7},
    {:num => 8, :name => "8", :id => 8},
    {:num => 9, :name => "9", :id => 9}],
   {:upsert => true}
 )),
 (r.db("6570430476421813371").table("7953218286624157490")),
 (r.db("6570430476421813371").table("7953218286624157490").insert(
   {:broken => true, :id => 0},
   {:upsert => true}
 )),
 (r.db("6570430476421813371").table("7953218286624157490").insert(
   {:broken => true, :id => 1},
   {:upsert => true}
 )),
 (r.db("6570430476421813371").table("7953218286624157490")),
 (r.db("6570430476421813371").table("7953218286624157490")),
 (r.db("6570430476421813371").table("7953218286624157490")),
 (r.db("6570430476421813371").table("7953218286624157490").filter {|var_101|
   var_101["id"].eq(0)
 }.replace { {:num => 0, :name => "0", :id => 0}}),
 (r.db("6570430476421813371").table("7953218286624157490").update {|var_102|
   var_102.contains("broken").branch(
     {:num => 1, :name => "1", :id => 1},
     var_102
   )
 }),
 (r.db("6570430476421813371").table("7953218286624157490").replace {|var_103|
   {:num => var_103["num"], :name => var_103["name"], :id => var_103["id"]}
 }),
 (r.db("6570430476421813371").table("7953218286624157490")),
 (r.db("6570430476421813371").table("7953218286624157490").replace {|var_104|
   var_104["id"].eq(4).any(var_104["id"].eq(5)).branch({:id => -1}, var_104)
 }),
 (r.db("6570430476421813371").table("7953218286624157490").get(5).update {
   {:num => "5"}
 }),
 (r.db("6570430476421813371").table("7953218286624157490").replace {|var_105|
   var_105.merge({:num => var_105["num"].mul(2)})
 }),
 (r.db("6570430476421813371").table("7953218286624157490").get(5)),
 (r.db("6570430476421813371").table("7953218286624157490").replace {|var_106|
   var_106["id"].eq(5).branch(
     {:num => 5, :name => "5", :id => 5},
     var_106.merge({:num => var_106["num"].div(2)})
   )
 }),
 (r.db("6570430476421813371").table("7953218286624157490")),
 (r.db("6570430476421813371").table("7953218286624157490").get(0).update {
   {:num => 2}
 }),
 (r.db("6570430476421813371").table("7953218286624157490").get(0)),
 (r.db("6570430476421813371").table("7953218286624157490").get(0).update { nil}),
 (r.db("6570430476421813371").table("7953218286624157490").get(0).replace {
   {:num => 0, :name => "0", :id => 0}
 }),
 (r.db("6570430476421813371").table("7953218286624157490")),
 (r.db("6570430476421813371").table("7953218286624157490").filter {|var_107|
   var_107["id"].lt(5)
 }.delete),
 (r.db("6570430476421813371").table("7953218286624157490")),
 (r.db("6570430476421813371").table("7953218286624157490").delete),
 (r.db("6570430476421813371").table("7953218286624157490")),
 (r.db("6570430476421813371").table("7953218286624157490").insert(
   [{:num => 0, :name => "0", :id => 0},
    {:num => 1, :name => "1", :id => 1},
    {:num => 2, :name => "2", :id => 2},
    {:num => 3, :name => "3", :id => 3},
    {:num => 4, :name => "4", :id => 4},
    {:num => 5, :name => "5", :id => 5},
    {:num => 6, :name => "6", :id => 6},
    {:num => 7, :name => "7", :id => 7},
    {:num => 8, :name => "8", :id => 8},
    {:num => 9, :name => "9", :id => 9}],
   {:upsert => true}
 )),
 (r.db("6570430476421813371").table("7953218286624157490").insert(
   [{:num => 0, :name => "0", :id => 0},
    {:num => 1, :name => "1", :id => 1},
    {:num => 2, :name => "2", :id => 2},
    {:num => 3, :name => "3", :id => 3},
    {:num => 4, :name => "4", :id => 4},
    {:num => 5, :name => "5", :id => 5},
    {:num => 6, :name => "6", :id => 6},
    {:num => 7, :name => "7", :id => 7},
    {:num => 8, :name => "8", :id => 8},
    {:num => 9, :name => "9", :id => 9}],
   {:upsert => true}
 )),
 (r.db("6570430476421813371").table("7953218286624157490")),
 (r.db("6570430476421813371").table("7953218286624157490").replace {|var_108|
   var_108
 }),
 (r.db("6570430476421813371").table("7953218286624157490")),
 (r.db("6570430476421813371").table("7953218286624157490").replace {|var_109|
   var_109["id"].lt(5).branch(nil, var_109)
 }),
 (r.db("6570430476421813371").table("7953218286624157490")),
 (r.db("6570430476421813371").table("7953218286624157490").insert(
   [{:num => 0, :name => "0", :id => 0},
    {:num => 1, :name => "1", :id => 1},
    {:num => 2, :name => "2", :id => 2},
    {:num => 3, :name => "3", :id => 3},
    {:num => 4, :name => "4", :id => 4}],
   {:upsert => true}
 )),
 (r.db("6570430476421813371").table("7953218286624157490").foreach {|var_110|
   [r.db("6570430476421813371").table("7953218286624157490").get(
      var_110["id"]
    ).delete,
    r.db("6570430476421813371").table("7953218286624157490").insert(
      var_110,
      {:upsert => true}
    )]
 }),
 (r.db("6570430476421813371").table("7953218286624157490")),
 (r.db("6570430476421813371").table("7953218286624157490").foreach {|var_111|
   r.db("6570430476421813371").table("7953218286624157490").get(
     var_111["id"]
   ).delete
 }),
 (r.db("6570430476421813371").table("7953218286624157490")),
 (r.db("6570430476421813371").table("7953218286624157490").insert(
   [{:num => 0, :name => "0", :id => 0},
    {:num => 1, :name => "1", :id => 1},
    {:num => 2, :name => "2", :id => 2},
    {:num => 3, :name => "3", :id => 3},
    {:num => 4, :name => "4", :id => 4},
    {:num => 5, :name => "5", :id => 5},
    {:num => 6, :name => "6", :id => 6},
    {:num => 7, :name => "7", :id => 7},
    {:num => 8, :name => "8", :id => 8},
    {:num => 9, :name => "9", :id => 9}],
   {:upsert => true}
 )),
 (r.db("6570430476421813371").table("7953218286624157490")),
 (r.db("6570430476421813371").table("7953218286624157490").get(0).update {
   {:broken => 5, :id => 0}
 }),
 (r.db("6570430476421813371").table("7953218286624157490").insert(
   {:num => 0, :name => "0", :id => 0},
   {:upsert => true}
 )),
 (r.db("6570430476421813371").table("7953218286624157490")),
 (r.db("6570430476421813371").table("7953218286624157490").get(0).replace { nil}),
 (r.db("6570430476421813371").table("7953218286624157490")),
 (r.db("6570430476421813371").table("7953218286624157490").get(1).replace {
   {:id => -1}
 }),
 (r.db("6570430476421813371").table("7953218286624157490").get(
   1
 ).replace {|var_112| {:name => var_112["name"].mul(2)}}),
 (r.db("6570430476421813371").table("7953218286624157490").get(
   1
 ).replace {|var_113| var_113.merge({:num => 2})}),
 (r.db("6570430476421813371").table("7953218286624157490").get(1)),
 (r.db("6570430476421813371").table("7953218286624157490").insert(
   [{:num => 0, :name => "0", :id => 0}, {:num => 1, :name => "1", :id => 1}],
   {:upsert => true}
 )),
 (r.db("6570430476421813371").table("7953218286624157490")),
 (r("6570430476421813371").db_drop),
 (r("test").db_create),
 (r.db("test").table_create("tbl")),
 (r.db("test").table("tbl").delete),
 (r.db("test").table("tbl").insert(
   [{:num => 0, :name => "0", :id => 0},
    {:num => 1, :name => "1", :id => 1},
    {:num => 2, :name => "2", :id => 2},
    {:num => 3, :name => "3", :id => 3},
    {:num => 4, :name => "4", :id => 4},
    {:num => 5, :name => "5", :id => 5},
    {:num => 6, :name => "6", :id => 6},
    {:num => 7, :name => "7", :id => 7},
    {:num => 8, :name => "8", :id => 8},
    {:num => 9, :name => "9", :id => 9}]
 )),
 (r.db("test").table("tbl").delete),
 (r.db("test").table("tbl").insert(
   [{:b => 10, :a => 3, :id => 1},
    {:b => -5, :a => 9, :id => 2},
    {:b => 3, :a => 9, :id => 3}]
 )),
 (r.db("test").table("tbl").get(1)),
 (r.db("test").table("tbl").get(2)),
 (r.db("test").table("tbl").get(3)),
 (r.db("test").table("tbl").filter({:a => 3})),
 (r.db("test").table("tbl").filter(
   {:a => r.db("test").table("tbl").count.add("")}
 )),
 (r.db("test").table("tbl").get(0)),
 (r.db("test").table("tbl").insert(
   {:text => "u{30b0}u{30eb}u{30e1}", :id => 100}
 )),
 (r.db("test").table("tbl").get(100)["text"]),
 (r.db("test").table("tbl").delete),
 (r.db("test").table("tbl").insert([{:id => 1}, {:id => 2}])),
 (r.db("test").table("tbl").limit(1).delete),
 (r.db("test").table("tbl")),
 (r.db("test").table("tbl").delete),
 (r.db("test").table("tbl").insert(
   [{:b => 0, :a => 0, :id => 100},
    {:b => 1, :a => 1, :id => 101},
    {:b => 2, :a => 2, :id => 102},
    {:b => 0, :a => 3, :id => 103}]
 )),
 (r.db("test").table("tbl").get(100)),
 (r.db("test").table("tbl").get(101)),
 (r.db("test").table("tbl").get(102)),
 (r.db("test").table("tbl").get(103)),
 (r.db("test").table("tbl").delete),
 (r.db("test").table("tbl").insert(
   [{:b => 0, :a => 0, :id => 100},
    {:b => 1, :a => 1, :id => 101},
    {:b => 2, :a => 2, :id => 102},
    {:b => 0, :a => 3, :id => 103},
    {:b => 1, :a => 4, :id => 104},
    {:b => 2, :a => 5, :id => 105},
    {:b => 0, :a => 6, :id => 106},
    {:b => 1, :a => 7, :id => 107},
    {:b => 2, :a => 8, :id => 108},
    {:b => 0, :a => 9, :id => 109}]
 )),
 (r.db("test").table("tbl").filter {|var_114| var_114["a"].lt(5)}),
 (r.db("test").table("tbl").filter {|var_115| var_115["a"].le(5)}),
 (r.db("test").table("tbl").filter {|var_116| var_116["a"].gt(5)}),
 (r.db("test").table("tbl").filter {|var_117| var_117["a"].ge(5)}),
 (r.db("test").table("tbl").filter {|var_118| var_118["a"].eq(var_118["b"])}),
 (r(-5)),
 (r(5)),
 (r(5)),
 (r(5)),
 (r(3).add(4)),
 (r(3).sub(4)),
 (r(3).mul(4)),
 (r(3).div(4)),
 (r(3).mod(2)),
 (r(4).add(3)),
 (r(4).sub(3)),
 (r(4).mul(3)),
 (r(4).div(4)),
 (r(4).mod(3)),
 (r(3).add(4).mul(r(0).sub(6)).mul(r(-5).add(3))),
 (r.db("test").table("tbl").delete),
 (r.db("test").table("tbl").insert(
   [{:b => 0, :a => 0, :id => 100},
    {:b => 1, :a => 1, :id => 101},
    {:b => 2, :a => 2, :id => 102},
    {:b => 0, :a => 3, :id => 103},
    {:b => 1, :a => 4, :id => 104},
    {:b => 2, :a => 5, :id => 105},
    {:b => 0, :a => 6, :id => 106},
    {:b => 1, :a => 7, :id => 107},
    {:b => 2, :a => 8, :id => 108},
    {:b => 0, :a => 9, :id => 109}]
 )),
 (r.db("test").table("tbl").replace {|var_119| var_119}),
 (r.db("test").table("tbl")),
 (r.db("test").table("tbl").update { nil}),
 (r.db("test").table("tbl").update { 1}),
 (r.db("test").table("tbl").update {|var_120| var_120["id"]}),
 (r.db("test").table("tbl").update {|var_121| var_121["id"]}),
 (r.db("test").table("tbl").update {|var_122|
   var_122["id"].mod(2).eq(0).branch({:id => -1}, var_122)
 }),
 (r.db("test").table("tbl")),
 (r.db("test").table_create("tbl")),
 (r.db("test").table("tbl").delete),
 (r.db("test").table("tbl").insert(
   [{:id => 0},
    {:id => 1},
    {:id => 2},
    {:id => 3},
    {:id => 4},
    {:id => 5},
    {:id => 6},
    {:id => 7},
    {:id => 8},
    {:id => 9}]
 )),
 (r.db("test").table("tbl").update {|var_123|
   {:count => r.db("test").table("tbl").get(0).map {|var_124| 0}}
 }),
 (r.db("test").table("tbl").update {|var_125| {:count => 0}}),
 (r.db("test").table("tbl").replace {|var_126|
   r.db("test").table("tbl").get(var_126["id"])
 }),
 (r.db("test").table("tbl").update {
   {:count =>
      r.db("test").table("tbl").map {|var_127|
        var_127["count"]
      }.reduce({:base => 0}) {|var_128, var_129| var_128.add(var_129)}}
 }),
 (r.db("test").table("tbl").orderby("id")),
 (r.db("test").table("tbl").update {
   {:count =>
      r([{:id => 0.0, :count => 0.0},
       {:id => 1.0, :count => 0.0},
       {:id => 2.0, :count => 0.0},
       {:id => 3.0, :count => 0.0},
       {:id => 4.0, :count => 0.0},
       {:id => 5.0, :count => 0.0},
       {:id => 6.0, :count => 0.0},
       {:id => 7.0, :count => 0.0},
       {:id => 8.0, :count => 0.0},
       {:id => 9.0, :count => 0.0}]).map {|var_130|
        var_130["id"]
      }.reduce({:base => 0}) {|var_131, var_132| var_131.add(var_132)}}
 }),
 (r.db("test").table("tbl").map {|var_133| var_133["count"]}.reduce({:base => 0}
 ) {|var_134, var_135| var_134.add(var_135)}),
 (r.db("test").table("tbl").map {|var_136| var_136["id"]}.reduce({:base => 0}
 ) {|var_137, var_138| var_137.add(var_138)}.mul(
   r.db("test").table("tbl").count
 )),
 (r.db("test").table("tbl").update {|var_139|
   {:x => lambda { 1}.funcall(r.db("test").table("tbl").get(0))}
 }),
 (r.db("test").table("tbl").update({:non_atomic => true}) {|var_140|
   {:x => lambda { 1}.funcall(r.db("test").table("tbl").get(0))}
 }),
 (r.db("test").table("tbl").map {|var_141| var_141["x"]}.reduce({:base => 0}
 ) {|var_142, var_143| var_142.add(var_143)}),
 (r.db("test").table("tbl").get(0).update {|var_144|
   {:x => lambda { 1}.funcall(r.db("test").table("tbl").get(0))}
 }),
 (r.db("test").table("tbl").get(0).update({:non_atomic => true}) {|var_145|
   {:x => lambda { 2}.funcall(r.db("test").table("tbl").get(0))}
 }),
 (r.db("test").table("tbl").map {|var_146| var_146["x"]}.reduce({:base => 0}
 ) {|var_147, var_148| var_147.add(var_148)}),
 (r.db("test").table("tbl").update { r("").error}),
 (r.db("test").table("tbl").map {|var_149| var_149["x"]}.reduce({:base => 0}
 ) {|var_150, var_151| var_150.add(var_151)}),
 (r.db("test").table("tbl").update {|var_152|
   lambda { true}.funcall(r.db("test").table("tbl").get(0)).branch(
     nil,
     {:x => 0.1}
   )
 }),
 (r.db("test").table("tbl").update({:non_atomic => true}) {|var_153|
   lambda { true}.funcall(r.db("test").table("tbl").get(0)).branch(
     nil,
     {:x => 0.1}
   )
 }),
 (r.db("test").table("tbl").map {|var_154| var_154["x"]}.reduce({:base => 0}
 ) {|var_155, var_156| var_155.add(var_156)}),
 (r.db("test").table("tbl").get(0).update {
   lambda { true}.funcall(r.db("test").table("tbl").get(0)).branch(
     nil,
     {:x => 0.1}
   )
 }),
 (r.db("test").table("tbl").get(0).update({:non_atomic => true}) {
   lambda { true}.funcall(r.db("test").table("tbl").get(0)).branch(
     nil,
     {:x => 0.1}
   )
 }),
 (r.db("test").table("tbl").map {|var_157| var_157["x"]}.reduce({:base => 0}
 ) {|var_158, var_159| var_158.add(var_159)}),
 (r.db("test").table("tbl").get(0).replace {|var_160|
   lambda { true}.funcall(r.db("test").table("tbl").get(0)).branch(var_160, nil)
 }),
 (r.db("test").table("tbl").get(0).replace({:non_atomic => true}) {|var_161|
   lambda { true}.funcall(r.db("test").table("tbl").get(0)).branch(var_161, nil)
 }),
 (r.db("test").table("tbl").map {|var_162| var_162["x"]}.reduce({:base => 0}
 ) {|var_163, var_164| var_163.add(var_164)}),
 (r.db("test").table("tbl").replace {|var_165|
   lambda { var_165}.funcall(r.db("test").table("tbl").get(0))
 }),
 (r.db("test").table("tbl").replace({:non_atomic => true}) {|var_166|
   lambda { var_166}.funcall(r.db("test").table("tbl").get(0))["id"].eq(
     1
   ).branch(var_166.merge({:x => 2}), var_166)
 }),
 (r.db("test").table("tbl").map {|var_167| var_167["x"]}.reduce({:base => 0}
 ) {|var_168, var_169| var_168.add(var_169)}),
 (r.db("test").table("tbl").get(0).replace {|var_170|
   lambda { r("").error}.funcall(r.db("test").table("tbl").get(0)).branch(
     var_170,
     nil
   )
 }),
 (r.db("test").table("tbl").get(0).replace({:non_atomic => true}) {|var_171|
   lambda { r("").error}.funcall(r.db("test").table("tbl").get(0)).branch(
     var_171,
     nil
   )
 }),
 (r.db("test").table("tbl").map {|var_172| var_172["x"]}.reduce({:base => 0}
 ) {|var_173, var_174| var_173.add(var_174)}),
 (r.db("test").table("tbl").replace {|var_175|
   lambda { r("").error}.funcall(r.db("test").table("tbl").get(0)).branch(
     var_175,
     nil
   )
 }),
 (r.db("test").table("tbl").replace({:non_atomic => true}) {|var_176|
   lambda { r("").error}.funcall(r.db("test").table("tbl").get(0)).branch(
     var_176,
     nil
   )
 }),
 (r.db("test").table("tbl").map {|var_177| var_177["x"]}.reduce({:base => 0}
 ) {|var_178, var_179| var_178.add(var_179)}),
 (r.db("test").table("tbl").get(0).replace {|var_180|
   lambda { true}.funcall(r.db("test").table("tbl").get(0)).branch(nil, var_180)
 }),
 (r.db("test").table("tbl").get(0).replace({:non_atomic => true}) {|var_181|
   lambda { true}.funcall(r.db("test").table("tbl").get(0)).branch(nil, var_181)
 }),
 (r.db("test").table("tbl").map {|var_182| var_182["x"]}.reduce({:base => 0}
 ) {|var_183, var_184| var_183.add(var_184)}),
 (r.db("test").table("tbl").replace {|var_185|
   lambda { var_185}.funcall(r.db("test").table("tbl").get(0))["id"].lt(
     3
   ).branch(nil, var_185)
 }),
 (r.db("test").table("tbl").replace({:non_atomic => true}) {|var_186|
   lambda { var_186}.funcall(r.db("test").table("tbl").get(0))["id"].lt(
     3
   ).branch(nil, var_186)
 }),
 (r.db("test").table("tbl").map {|var_187| var_187["x"]}.reduce({:base => 0}
 ) {|var_188, var_189| var_188.add(var_189)}),
 (r.db("test").table("tbl").get(0).replace {
   {:count =>
      r.db("test").table("tbl").get(3)["count"],
    :x =>
      r.db("test").table("tbl").get(3)["x"],
    :id =>
      0}
 }),
 (r.db("test").table("tbl").get(0).replace({:non_atomic => true}) {
   {:count =>
      r.db("test").table("tbl").get(3)["count"],
    :x =>
      r.db("test").table("tbl").get(3)["x"],
    :id =>
      0}
 }),
 (r.db("test").table("tbl").get(1).replace {
   r.db("test").table("tbl").get(3).merge({:id => 1})
 }),
 (r.db("test").table("tbl").get(1).replace({:non_atomic => true}) {
   r.db("test").table("tbl").get(3).merge({:id => 1})
 }),
 (r.db("test").table("tbl").get(2).replace({:non_atomic => true}) {
   r.db("test").table("tbl").get(1).merge({:id => 2})
 }),
 (r.db("test").table("tbl").map {|var_190| var_190["x"]}.reduce({:base => 0}
 ) {|var_191, var_192| var_191.add(var_192)})]
