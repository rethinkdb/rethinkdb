"Specifies the initial simulation data."

import shelve
from random import randrange
import atexit

data = shelve.open('simulation-data', writeback=True)

def reset_data():
    "Resets the simulation data."

    for key in data.keys():
        del key

    data['datacenters'] = {
        '01f04592-e403-4abc-a845-83d43f6fd967': {
            'name': 'europe_1',
            'health': '75',
            'status': 'Available',
            'id': '01f04592-e403-4abc-a845-83d43f6fd967',
        },

        '1e7509ad-8143-4cbb-bdbd-6db3d6766fc0': {
            'name': 'usa_1',
            'health': '90',
            'status': 'Unavailable',
            'id': '1e7509ad-8143-4cbb-bdbd-6db3d6766fc0',
        },

        '0a0e8afe-9d4d-41b8-bdd7-651c535d54af': {
            'name': 'europe_2',
            'health': '30',
            'status': 'Available',
            'id': '0a0e8afe-9d4d-41b8-bdd7-651c535d54af',
        },

        '343970b8-28f6-46bb-8106-b735fda9c954': {
            'name': 'usa_2',
            'health': '80',
            'status': 'Available',
            'id': '343970b8-28f6-46bb-8106-b735fda9c954',
        },

        '2c8eb43c-4783-4df7-805e-b81ab7bc2f03': {
            'name': 'greece',
            'health': '5',
            'status': 'Available',
            'id': '2c8eb43c-4783-4df7-805e-b81ab7bc2f03',
        },

    }

    # TODO: change the rest
    data['namespaces'] = {
        'a9f1eeed-6893-4e5a-99fa-914b78fbaaa2': {
            'name': 'users',
            'protocol': 'memcached',
            'id': 'a9f1eeed-6893-4e5a-99fa-914b78fbaaa2',
            'primary_uuid': '1e7509ad-8143-4cbb-bdbd-6db3d6766fc0',
            'replica_affinities': {
                '1e7509ad-8143-4cbb-bdbd-6db3d6766fc0': {
                    'desired_replication_count': 4,
                    'machines': [ [ 'e50d0c6b-c23b-474b-9514-ec0112dd96be',
                                    'cc0e5d39-36b2-41f1-8a6d-0e5ace996aae',
                                    '445f636e-22bc-40f2-bb82-84b1dc78021c',
                                    '2239dc93-039c-411a-9775-5b1d73132104' ] ]
                    },
                '343970b8-28f6-46bb-8106-b735fda9c954': {
                    'desired_replication_count': 1,
                    'machines': [ [ 'e50d0c6b-c23b-474b-9514-ec0112dd96be' ] ]
                    },
                '01f04592-e403-4abc-a845-83d43f6fd967': {
                    'desired_replication_count': 2,
                    'machines': [ [ 'e50d0c6b-c23b-474b-9514-ec0112dd96be',
                                    'cc0e5d39-36b2-41f1-8a6d-0e5ace996aae' ] ]
                    },
                '0a0e8afe-9d4d-41b8-bdd7-651c535d54af': {
                    'desired_replication_count': 3,
                    'machines': [ [ 'e50d0c6b-c23b-474b-9514-ec0112dd96be',
                                    'cc0e5d39-36b2-41f1-8a6d-0e5ace996aae',
                                    '445f636e-22bc-40f2-bb82-84b1dc78021c' ] ]
                    },
                },
            'shards': []
        },

        'b14bf374-4267-487d-8108-50a792969600': {
            'name': 'tweets',
            'protocol': 'memcached',
            'id': 'b14bf374-4267-487d-8108-50a792969600',
            'primary_uuid': '1e7509ad-8143-4cbb-bdbd-6db3d6766fc0',
            'replica_affinities': {
                '1e7509ad-8143-4cbb-bdbd-6db3d6766fc0': {
                    'desired_replication_count': 4,
                    'machines': [ [ 'e50d0c6b-c23b-474b-9514-ec0112dd96be',
                                    'cc0e5d39-36b2-41f1-8a6d-0e5ace996aae',
                                    '445f636e-22bc-40f2-bb82-84b1dc78021c',
                                    '2239dc93-039c-411a-9775-5b1d73132104' ] ]
                    },
                '343970b8-28f6-46bb-8106-b735fda9c954': {
                    'desired_replication_count': 1,
                    'machines': [ [ 'e50d0c6b-c23b-474b-9514-ec0112dd96be' ] ]
                    },
                '01f04592-e403-4abc-a845-83d43f6fd967': {
                    'desired_replication_count': 2,
                    'machines': [ [ 'e50d0c6b-c23b-474b-9514-ec0112dd96be',
                                    'cc0e5d39-36b2-41f1-8a6d-0e5ace996aae' ] ]
                    },
                '0a0e8afe-9d4d-41b8-bdd7-651c535d54af': {
                    'desired_replication_count': 3,
                    'machines': [ [ 'e50d0c6b-c23b-474b-9514-ec0112dd96be',
                                    'cc0e5d39-36b2-41f1-8a6d-0e5ace996aae',
                                    '445f636e-22bc-40f2-bb82-84b1dc78021c' ] ]
                    },
                },
            'shards': []
        },

        '9bb870bf-bbcd-456f-8240-12d18bb13cc5': {
            'name': 'photo_metadata',
            'protocol': 'riak',
            'id': '9bb870bf-bbcd-456f-8240-12d18bb13cc5',
            'primary_uuid': '1e7509ad-8143-4cbb-bdbd-6db3d6766fc0',
            'replica_affinities': {
                '1e7509ad-8143-4cbb-bdbd-6db3d6766fc0': {
                    'desired_replication_count': 4,
                    'machines': [ [ 'e50d0c6b-c23b-474b-9514-ec0112dd96be',
                                    'cc0e5d39-36b2-41f1-8a6d-0e5ace996aae',
                                    '445f636e-22bc-40f2-bb82-84b1dc78021c',
                                    '2239dc93-039c-411a-9775-5b1d73132104' ] ]
                    },
                '343970b8-28f6-46bb-8106-b735fda9c954': {
                    'desired_replication_count': 1,
                    'machines': [ [ 'e50d0c6b-c23b-474b-9514-ec0112dd96be' ] ]
                    },
                '01f04592-e403-4abc-a845-83d43f6fd967': {
                    'desired_replication_count': 2,
                    'machines': [ [ 'e50d0c6b-c23b-474b-9514-ec0112dd96be',
                                    'cc0e5d39-36b2-41f1-8a6d-0e5ace996aae' ] ]
                    },
                '0a0e8afe-9d4d-41b8-bdd7-651c535d54af': {
                    'desired_replication_count': 3,
                    'machines': [ [ 'e50d0c6b-c23b-474b-9514-ec0112dd96be',
                                    'cc0e5d39-36b2-41f1-8a6d-0e5ace996aae',
                                    '445f636e-22bc-40f2-bb82-84b1dc78021c' ] ]
                    },
                },
            'shards': []
        },

        '0ae6ef42-b699-48b2-9bc2-7a3e8e131b21': {
            'name': 'logs',
            'protocol': 'memcached',
            'id': '0ae6ef42-b699-48b2-9bc2-7a3e8e131b21',
            'primary_uuid': '1e7509ad-8143-4cbb-bdbd-6db3d6766fc0',
            'replica_affinities': {
                '1e7509ad-8143-4cbb-bdbd-6db3d6766fc0': {
                    'desired_replication_count': 4,
                    'machines': [ [ 'e50d0c6b-c23b-474b-9514-ec0112dd96be',
                                    'cc0e5d39-36b2-41f1-8a6d-0e5ace996aae',
                                    '445f636e-22bc-40f2-bb82-84b1dc78021c',
                                    '2239dc93-039c-411a-9775-5b1d73132104' ] ]
                    },
                '343970b8-28f6-46bb-8106-b735fda9c954': {
                    'desired_replication_count': 1,
                    'machines': [ [ 'e50d0c6b-c23b-474b-9514-ec0112dd96be' ] ]
                    },
                '01f04592-e403-4abc-a845-83d43f6fd967': {
                    'desired_replication_count': 2,
                    'machines': [ [ 'e50d0c6b-c23b-474b-9514-ec0112dd96be',
                                    'cc0e5d39-36b2-41f1-8a6d-0e5ace996aae' ] ]
                    },
                '0a0e8afe-9d4d-41b8-bdd7-651c535d54af': {
                    'desired_replication_count': 3,
                    'machines': [ [ 'e50d0c6b-c23b-474b-9514-ec0112dd96be',
                                    'cc0e5d39-36b2-41f1-8a6d-0e5ace996aae',
                                    '445f636e-22bc-40f2-bb82-84b1dc78021c' ] ]
                    },
                },
            'shards': []
        },
    }

    data['machines'] = {
        'e50d0c6b-c23b-474b-9514-ec0112dd96be': {
            'name': 'magneto',
            'ip': '192.168.15',
            'datacenter_uuid': '01f04592-e403-4abc-a845-83d43f6fd967',
            'status': 'Available',
            'uptime': '6 days, 2:54',
            'cpu': randrange(0,100),
            'used_ram': '631',
            'total_ram': '2048',
            'used_disk': '842190',
            'total_disk': '1000000',
            'id': 'e50d0c6b-c23b-474b-9514-ec0112dd96be',
        },

        'cc0e5d39-36b2-41f1-8a6d-0e5ace996aae': {
            'name': 'electro',
            'ip': '192.168.7',
            'datacenter_uuid': '01f04592-e403-4abc-a845-83d43f6fd967',
            'status': 'Unavailable',
            'uptime': '7 days, 3:15',
            'cpu': randrange(0,100),
            'used_ram': '174',
            'total_ram': '2048',
            'used_disk': '135283',
            'total_disk': '1000000',
            'id': 'cc0e5d39-36b2-41f1-8a6d-0e5ace996aae',
        },

        '445f636e-22bc-40f2-bb82-84b1dc78021c': {
            'name': 'deadshot',
            'ip': '192.168.3',
            'datacenter_uuid': '0a0e8afe-9d4d-41b8-bdd7-651c535d54af',
            'status': 'Unavailable',
            'uptime': '4 days, 7:12',
            'cpu': randrange(0,100),
            'used_ram': '886',
            'total_ram': '2048',
            'used_disk': '129427',
            'total_disk': '1000000',
            'id': '445f636e-22bc-40f2-bb82-84b1dc78021c',
        },

        '2239dc93-039c-411a-9775-5b1d73132104': {
            'name': 'puzzler',
            'ip': '192.168.9',
            'datacenter_uuid': '343970b8-28f6-46bb-8106-b735fda9c954',
            'status': 'Available',
            'uptime': '3 days, 2:59',
            'cpu': randrange(0,100),
            'used_ram': '567',
            'total_ram': '2048',
            'used_disk': '863003',
            'total_disk': '1000000',
            'id': '2239dc93-039c-411a-9775-5b1d73132104',
        },

        '51ad340e-97ec-4e08-b451-d1b1f966ec30': {
            'name': 'riddler',
            'ip': '192.168.11',
            'datacenter_uuid': '01f04592-e403-4abc-a845-83d43f6fd967',
            'status': 'Available',
            'uptime': '2 days, 3:22',
            'cpu': randrange(0,100),
            'used_ram': '763',
            'total_ram': '2048',
            'used_disk': '993600',
            'total_disk': '1000000',
            'id': '51ad340e-97ec-4e08-b451-d1b1f966ec30',
        },

        'b0939d6b-e048-4e6e-9866-11a93451d8f2': {
            'name': 'alphar',
            'ip': '192.168.12',
            'datacenter_uuid': '2c8eb43c-4783-4df7-805e-b81ab7bc2f03',
            'status': 'Available',
            'uptime': '2 days, 3:22',
            'cpu': randrange(0,100),
            'used_ram': '763',
            'total_ram': '2048',
            'used_disk': '993600',
            'total_disk': '1000000',
            'id': 'b0939d6b-e048-4e6e-9866-11a93451d8f2',
        },

        '70291f99-d1ac-4fc0-95a9-55791616f8ec': {
            'name': 'betar',
            'ip': '192.168.13',
            'datacenter_uuid': '2c8eb43c-4783-4df7-805e-b81ab7bc2f03',
            'status': 'Available',
            'uptime': '2 days, 3:22',
            'cpu': randrange(0,100),
            'used_ram': '763',
            'total_ram': '2048',
            'used_disk': '993600',
            'total_disk': '1000000',
            'id': '70291f99-d1ac-4fc0-95a9-55791616f8ec',
        },

        '046e3841-c89e-46db-9ea7-a3c259cc3dea': {
            'name': 'gammar',
            'ip': '192.168.14',
            'datacenter_uuid': '2c8eb43c-4783-4df7-805e-b81ab7bc2f03',
            'status': 'Available',
            'uptime': '2 days, 3:22',
            'cpu': randrange(0,100),
            'used_ram': '763',
            'total_ram': '2048',
            'used_disk': '993600',
            'total_disk': '1000000',
            'id': '046e3841-c89e-46db-9ea7-a3c259cc3dea',
        },
        '3b5ae567-5d6e-4747-a45a-dd6349654f20': {
            'name': 'deltar',
            'ip': '192.168.15',
            'datacenter_uuid': '2c8eb43c-4783-4df7-805e-b81ab7bc2f03',
            'status': 'Available',
            'uptime': '2 days, 3:22',
            'cpu': randrange(0,100),
            'used_ram': '763',
            'total_ram': '2048',
            'used_disk': '993600',
            'total_disk': '1000000',
            'id': '3b5ae567-5d6e-4747-a45a-dd6349654f20',
        },
        '58853886-300a-49f0-8d15-280c2a07f49c': {
            'name': 'episilore',
            'ip': '192.168.16',
            'datacenter_uuid': '2c8eb43c-4783-4df7-805e-b81ab7bc2f03',
            'status': 'Available',
            'uptime': '2 days, 3:22',
            'cpu': randrange(0,100),
            'used_ram': '763',
            'total_ram': '2048',
            'used_disk': '993600',
            'total_disk': '1000000',
            'id': '58853886-300a-49f0-8d15-280c2a07f49c',
        },
        '80aac45e-29ed-4688-938f-4b0432719b73': {
            'name': 'zetar',
            'ip': '192.168.17',
            'datacenter_uuid': '2c8eb43c-4783-4df7-805e-b81ab7bc2f03',
            'status': 'Available',
            'uptime': '2 days, 3:22',
            'cpu': randrange(0,100),
            'used_ram': '763',
            'total_ram': '2048',
            'used_disk': '993600',
            'total_disk': '1000000',
            'id': '80aac45e-29ed-4688-938f-4b0432719b73',
        },
        '65d04678-8a1c-4d1b-9ffb-2afca47a31ac': {
            'name': 'etar',
            'ip': '192.168.18',
            'datacenter_uuid': '2c8eb43c-4783-4df7-805e-b81ab7bc2f03',
            'status': 'Available',
            'uptime': '2 days, 3:22',
            'cpu': randrange(0,100),
            'used_ram': '763',
            'total_ram': '2048',
            'used_disk': '993600',
            'total_disk': '1000000',
            'id': '65d04678-8a1c-4d1b-9ffb-2afca47a31ac',
        },
        '8ab02054-7e19-4db4-baed-67a42a8c451b': {
            'name': 'thetar',
            'ip': '192.168.19',
            'datacenter_uuid': '2c8eb43c-4783-4df7-805e-b81ab7bc2f03',
            'status': 'Available',
            'uptime': '2 days, 3:22',
            'cpu': randrange(0,100),
            'used_ram': '763',
            'total_ram': '2048',
            'used_disk': '993600',
            'total_disk': '1000000',
            'id': '8ab02054-7e19-4db4-baed-67a42a8c451b',
        },
        'c307c15a-94fc-4dfa-a087-4fc3203dbea6': {
            'name': 'iotar',
            'ip': '192.168.20',
            'datacenter_uuid': '2c8eb43c-4783-4df7-805e-b81ab7bc2f03',
            'status': 'Available',
            'uptime': '2 days, 3:22',
            'cpu': randrange(0,100),
            'used_ram': '763',
            'total_ram': '2048',
            'used_disk': '993600',
            'total_disk': '1000000',
            'id': 'c307c15a-94fc-4dfa-a087-4fc3203dbea6',
        }
    }


    data['alerts'] = {
        '1': {
            'type': 'warning',
            'message': 'Low disk space',
            'datetime': '8/13/2011 13:23:14',
            'applies_to': '51ad340e-97ec-4e08-b451-d1b1f966ec30',
        },

        '2': {
            'type': 'error',
            'message': 'Machine may be disconnected',
            'datetime': '8/13/2011 13:23:14',
            'applies_to': 'cc0e5d39-36b2-41f1-8a6d-0e5ace996aae',
        },

        '3': {
            'type': 'warning',
            'message': 'High CPU usage',
            'datetime': '8/13/2011 13:23:14',
            'applies_to': '51ad340e-97ec-4e08-b451-d1b1f966ec30',
        },

        '4': {
            'type': 'info',
            'message': 'Machine started',
            'datetime': '8/13/2011 13:23:14',
            'applies_to': '51ad340e-97ec-4e08-b451-d1b1f966ec30',
        },

        '5': {
            'type': 'info',
            'message': 'Machine shut down',
            'datetime': '8/13/2011 13:23:14',
            'applies_to': '51ad340e-97ec-4e08-b451-d1b1f966ec30',
        },

        '6': {
            'type': 'error',
            'message': 'Machine may be disconnected',
            'datetime': '8/16/2011 14:21:26',
            'applies_to': '445f636e-22bc-40f2-bb82-84b1dc78021c',
        },
    }

def close_shelf():
    "Closes data (for making sure we flush when we exit)."
    data.close()

atexit.register(close_shelf)
