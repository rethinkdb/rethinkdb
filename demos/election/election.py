from rethinkdb import *
import pprint
p = pprint.PrettyPrinter(indent = 4)
pp = p.pprint

#connect to the server.
connect("localhost", 28015+14850)

#The tables I have to work with.
pop = db("test").table("pop")
poll = db("test").table("polls")

#Join the two tables together.
pop_with_poll = pop.equi_join('Stname', poll).zip()

#States in which one party is within 12 points of the other party.
gop_is_close = pop_with_poll.filter(lambda x: (x["DEM"] - x["GOP"] < 12) & (x["DEM"] > x["GOP"]))
dem_is_close = pop_with_poll.filter(lambda x: (x["GOP"] - x["DEM"] < 12) & (x["GOP"] > x["DEM"]))

#Swing states
swing_states_gop = gop_is_close.map(lambda x: x["Stname"]).distinct()
swing_states_dem = dem_is_close.map(lambda x: x["Stname"]).distinct()

#How man voters either party would need to turn to win the states.
needed_to_win_gop = gop_is_close.grouped_map_reduce(lambda x: x["Stname"], lambda x: x['POPESTIMATE2010'] * ((x["DEM"] - x["GOP"]) / 100), 0, lambda x,y : x + y)
needed_to_win_dem = dem_is_close.grouped_map_reduce(lambda x: x["Stname"], lambda x: x['POPESTIMATE2010'] * ((x["GOP"] - x["DEM"]) / 100), 0, lambda x,y : x + y)

#Finally: for each voter turned how many electoral votes could one hope to gain.
return_on_investment_gop = needed_to_win_gop.equi_join("group", poll).zip().map(lambda x: {"State" : x["id"], "ev_per_person" : x["EV"] / x["reduction"]})
return_on_investment_dem = needed_to_win_dem.equi_join("group", poll).zip().map(lambda x: {"State" : x["id"], "ev_per_person" : x["EV"] / x["reduction"]})
