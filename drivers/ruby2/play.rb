# load 'quickstart2.rb'
# r.table('county_stats').eq_join('Stname', r.table('polls')).zip.pluck('Stname', 'state', 'county', 'ctyname', 'CENSUS2010POP', 'POPESTIMATE2011', 'Dem', 'GOP').filter {|doc|
#   doc['Dem'].lt(doc['GOP']).and(doc['GOP'] < doc['Dem']).lt(15)
# }.grouped_map_reduce(
#   lambda {|doc| doc['Stname']},
#   lambda {|doc| doc['POPESTIMATE2011'].mul(doc['GOP'].sub(doc['Dem']).div(100))},
#   0,
#   lambda {|acc, val| acc + val}
# ).orderby('reduction')

# puts $s.split("\n").map {|line|
#   arr = line.split("\0x43")
#   arr[1] ? [arr.join(""), " "*arr[0].size + "^"*arr[1].size] : line
# }.flatten.join("\n")
load 'quickstart2.rb'

# r.add(1000000, r.add(2000000, r.add(4000000, 5000000, 6000000), 7000000, 8000000, r.add(r.add(3000000, 9000000, 1000000, 2000000, r.add(3000000, 4000000, 5000000, 6000000)), "a"))).run

# r.add(1000000, r.add(2000000, r.add(4000000, 5000000, 6000000), r.add(3000000, "a"))).run

# r.add(1000000, r.add(2000000, r.add(4000000, 5000000, 6000000), 7000000, 8000000, r.add(3000000, "a"))).run

# r(10000000000000000000000000000000000000).run
