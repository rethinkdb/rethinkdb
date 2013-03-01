load 'quickstart2.rb'
r.table('county_stats').eq_join('Stname', r.table('polls')).zip.pluck('Stname', 'state', 'county', 'ctyname', 'CENSUS2010POP', 'POPESTIMATE2011', 'Dem', 'GOP').filter {|doc|
  doc['Dem'].lt(doc['GOP']).and(doc['GOP'] < doc['Dem']).lt(15)
}.grouped_map_reduce(
  lambda {|doc| doc['Stname']},
  lambda {|doc| doc['POPESTIMATE2011'].mul(doc['GOP'].sub(doc['Dem']).div(100))},
  0,
  lambda {|acc, val| acc + val}
).orderby('reduction')
