import rethinkdb

__all__ = ['r']

class SugaryModuleWrapper(object):
	def __init__(self, attrdict):
		self.__dict__ = attrdict

	def __getitem__(self, name):
		return rethinkdb.attr(name)

r = SugaryModuleWrapper(rethinkdb.__dict__)
