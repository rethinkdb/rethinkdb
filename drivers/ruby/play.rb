load 'quickstart2.rb'
r.db('test').table_list.run
r.db('test').table_create('test').run
r.db('test').table_create('test2').run
r.db('test').table_create('test3').run
# r.db('test').table_list.foreach {|x| r.db('test').table_drop(r.implicit_var)}.run
