"Backends"

import requests
import re
import json
import sys
from random import randrange
import uuid as python_uuid
from colorama import init, Fore
init()

class BackendError(Exception):
    "For errors that have happened.. in the backend."
    pass

def add_fake_data(route, data):
    "Adds random CPU data to machines"

    if re.search(r"^/machines/\w+", route):
        data['cpu'] = randrange(0, 100)
        data['iops'] = randrange(0, 10000)
    return data

def print_http_error(text):
    "Prints a red HTTP error to the console."
    print Fore.RED + text + Fore.WHITE

def print_json_data(data):
    "Prints a green HTTP error to the console."
    print Fore.GREEN + json.dumps(data) + Fore.WHITE

class SimulationBackend:
    "The useful simulation backend"

    def __init__(self):
        import simulation_data as simulation
        self.simulation = simulation

        if len(simulation.data.keys()) < 1:
            self.simulation.reset_data()

    # Get data from the backend
    def get(self, route):
        # If the route is simply '/', return the full state
        if route == '/':
            return dict(self.simulation.data)

        elements = route.strip('/').split('/')
        data = self.simulation.data

        # walk the dict until we get to the last attribute the route
        for i in range(len(elements)):
            data = data[elements[i]]

        # add any fake data, return the data
        return add_fake_data(route, data)

    # Add an element for a collection to the backend
    def add(self, collection, data):
        uuid = str(python_uuid.uuid4())
        data['id'] = uuid
        self.simulation.data[collection][uuid] = data
        return uuid

    # Set data for the backend
    def set(self, route, data):
        elements = route.strip('/').split('/')
        root = self.simulation.data

        # walk the dict until we get to the final top-level element
        for i in range(len(elements) - 1):
            root = root[elements[i]]

        attribute = elements[len(elements) - 1]

        root[attribute] = data
        return add_fake_data(route, root)

    # Delete data from the backend
    def delete(self, collection, uuid):
        del self.simulation.data[collection][uuid]
        return

    # Reset data on the backend
    def reset(self):
        self.simulation.reset_data()
        return


