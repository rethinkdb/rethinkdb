from __future__ import print_function

import os
import random
import time
import uuid

try:
	xrange
except NameError:
	xrange = range

def gen_doc(size_doc, i):
    if size_doc == "small":
        return {
            "field0": str(i // 1000),
            "field1": str(i),
        }
    elif size_doc == "big":
        # Size between 17 and 18k
        return {
            "field0": str(i // 1000),
            "field1": str(i),
            "string": str(uuid.uuid1()),
            "int": i,
            "float": i / 3.,
            "boolean": (random.random() > 0.5),
            "null": None,
            "array_num": [int(random.random() * 10000) for i in xrange(int(random.random() * 100))],
            "array_str": [str(uuid.uuid1()) for i in xrange(int(random.random() * 100))],
            "obj": {
                "nested0": str(uuid.uuid1()),
                "nested1": str(uuid.uuid1()),
                "nested2": str(uuid.uuid1())
            },
            "longstr1": "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Etiam tincidunt metus justo, in faucibus magna facilisis in. Sed adipiscing massa cursus, laoreet quam sed, dignissim urna. Nullam a pellentesque dolor. Aliquam nunc tortor, posuere ac tempus a, rhoncus non felis. Donec vel ante ornare, fermentum mauris quis, rhoncus nisi. Duis placerat nunc sit amet ipsum ultricies, eu euismod sapien fringilla. In id sapien ut arcu dignissim pellentesque sit amet non ante. Phasellus eget fermentum nunc, et condimentum libero.  Quisque porttitor, erat eget gravida feugiat, odio purus congue dui, nec varius purus turpis eget urna. Fusce facilisis est libero. Proin vitae libero vitae urna laoreet vulputate. Duis commodo, quam congue sodales posuere, neque ligula rhoncus nulla, cursus tristique neque ante et nunc. Donec placerat suscipit nulla vel faucibus. Vestibulum vehicula id diam eget feugiat. Donec vel diam fermentum, rutrum lectus id, vulputate dui.  Donec turpis risus, suscipit eu risus at, commodo suscipit massa. Quisque vel cursus leo, vitae tincidunt lacus. Vivamus fermentum tristique leo, vitae tempus diam faucibus eu. Nullam condimentum, est vitae vehicula facilisis, risus nulla viverra magna, quis elementum nunc nunc id mauris. Aliquam ante urna, volutpat accumsan lectus sit amet, scelerisque tristique orci. Sed sodales commodo purus ac ultrices. Mauris imperdiet ullamcorper luctus. Mauris faucibus metus a turpis blandit placerat. Donec interdum sem vitae quam convallis euismod.  Donec a magna elit. Vestibulum ante ipsum primis in faucibus orci luctus et ultrices posuere cubilia Curae; Ut blandit nisi augue, non porttitor dolor fringilla quis. Donec placerat a odio quis fringilla. Cras vitae aliquet nisl. Sed consequat dolor massa, et vulputate nibh dignissim eu. Donec dignissim cursus risus vel rutrum. Aliquam a faucibus nulla, sit amet blandit justo. Pellentesque id tortor sagittis, suscipit diam sed, imperdiet augue. Integer sit amet sem ac velit fermentum pharetra id a erat. In iaculis enim nec malesuada blandit.  Aenean malesuada sem non felis bibendum, blandit rhoncus turpis faucibus. Nam interdum massa dolor. Phasellus scelerisque rhoncus orci. Nullam hendrerit leo eget sem rutrum, viverra ultricies tortor congue. Suspendisse venenatis, augue id scelerisque molestie, dui arcu vestibulum eros, vitae facilisis augue massa at lectus. Maecenas at pulvinar magna. Suspendisse consequat diam vel augue molestie vehicula. Class aptent taciti sociosqu ad litora torquent per conubia nostra, per inceptos himenaeos. Vivamus ac commodo eros. Donec sit amet magna eget nibh dictum congue. Cras sapien odio, aliquam quis ullamcorper ut, interdum sed lectus. Aliquam risus justo, pellentesque vel magna in, fringilla porttitor magna. Pellentesque eleifend a augue nec rutrum. Nullam et lectus eu diam placerat semper. Pellentesque eget aliquam dui.  Nulla ultrices neque tincidunt, adipiscing leo eget, auctor augue. Sed ac metus convallis, consectetur eros eu, adipiscing lacus. Sed pellentesque ac sem nec tristique. Mauris imperdiet orci id nisl ullamcorper, non euismod erat tincidunt. Duis blandit facilisis dignissim. Quisque at tempus ligula. Cum sociis natoque penatibus et magnis dis parturient montes, nascetur ridiculus mus. Nullam tincidunt nibh felis, ut congue ligula lacinia nec. Sed ut ipsum vel elit tristique laoreet quis in diam. Etiam tempor erat eu aliquam tristique. Pellentesque habitant morbi tristique senectus et netus et malesuada fames ac turpis egestas. Nulla facilisi.  Maecenas cursus elit at varius lacinia. Etiam feugiat arcu sodales felis feugiat, et lobortis quam varius. Fusce et libero vitae dolor tincidunt tempor id ac lectus. Nam mollis viverra cursus. Nullam ut commodo mi, sit amet pretium lorem. Etiam tempus, velit sit amet lacinia lobortis, metus tellus vulputate orci, eu adipiscing metus dui et mauris. Nunc egestas consectetur nisi ut porta. Donec nec vehicula ligula. Nulla volutpat mi ac ornare elementum. Nullam risus justo, fringilla id tincidunt sit amet, elementum at purus. Cras a ullamcorper tellus, ac congue mi. Etiam malesuada leo a dui convallis pulvinar. Cum sociis natoque penatibus et magnis dis parturient montes, nascetur ridiculus mus. Morbi at ullamcorper nulla.  Curabitur eu molestie orci, porttitor feugiat quam. Pellentesque neque turpis, ullamcorper adipiscing scelerisque a, facilisis quis magna. Quisque nulla elit, luctus eget scelerisque non, scelerisque quis massa. Ut porttitor ante at mauris scelerisque adipiscing. Integer vel leo magna. Phasellus quam enim, malesuada et dignissim a, tempus id lorem. Nullam mattis tincidunt venenatis. Sed quam arcu, molestie sed ante vel, pulvinar fermentum mi. Nam malesuada id nibh sit amet dictum. Aliquam mi augue, mattis sit amet congue sed, dignissim ut odio.  Mauris scelerisque libero eget metus venenatis, ut mollis eros consectetur. Duis metus augue, molestie eget tincidunt vitae, volutpat vel lacus. Mauris fringilla imperdiet fermentum. Sed sit amet diam ut risus vulputate feugiat. Nulla vitae adipiscing quam. Duis non libero urna. Aenean ut ligula sed erat dictum dignissim aliquet non libero. Praesent quis neque varius lorem porta pulvinar. Integer aliquet elit vitae pretium mattis. Ut egestas nunc quis molestie commodo. Cras augue quam, cursus tristique sollicitudin sed, sagittis non velit. Lorem ipsum dolor sit amet, consectetur adipiscing elit. Donec malesuada in enim sed aliquam.  Curabitur lobortis fermentum purus. Maecenas vitae nibh ut libero congue interdum. Donec viverra ligula quis nibh volutpat, non luctus est dignissim. Suspendisse molestie, enim tempor consectetur gravida, ante sem porta mauris, a blandit velit quam suscipit justo. Etiam placerat euismod enim a rutrum. Praesent a imperdiet urna. Morbi quis vehicula leo. Nullam dictum fermentum nulla.  Mauris blandit pretium ultricies. Morbi ultrices est non sem suscipit mollis. Nam consequat ac ligula nec commodo. Ut mattis, tortor in laoreet tristique, quam dolor ornare massa, non luctus lacus ante eu massa. Nulla facilisi. Aliquam fringilla, felis non faucibus tempor, lorem sapien imperdiet mauris, rhoncus fermentum tellus nibh ut purus. Sed luctus risus quis mi interdum mollis. Duis sit amet nibh vel sem tincidunt vestibulum sed non eros. Duis laoreet orci dignissim est luctus, et pellentesque felis pulvinar.  Nam interdum massa eros, eu fringilla augue condimentum quis. Vestibulum pharetra mi quis felis hendrerit, eget malesuada nisl sagittis. Aliquam sit amet urna eu mauris dictum pharetra. Sed dignissim dignissim metus et elementum. Maecenas gravida lobortis tincidunt. Nulla dignissim, risus eu aliquam eleifend, lacus mi lobortis neque, sed venenatis erat ante at purus. Aliquam erat volutpat.  Nam eu eros a nisi mollis pretium vel vitae massa. Donec vulputate, ligula at fringilla ultrices, purus metus vehicula risus, ac sagittis purus metus sit amet libero. Curabitur eu dapibus urna, sed pharetra mauris. Mauris eget lacinia libero, vitae turpis duis.",
            "longstr2": "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Etiam tincidunt metus justo, in faucibus magna facilisis in. Sed adipiscing massa cursus, laoreet quam sed, dignissim urna. Nullam a pellentesque dolor. Aliquam nunc tortor, posuere ac tempus a, rhoncus non felis. Donec vel ante ornare, fermentum mauris quis, rhoncus nisi. Duis placerat nunc sit amet ipsum ultricies, eu euismod sapien fringilla. In id sapien ut arcu dignissim pellentesque sit amet non ante. Phasellus eget fermentum nunc, et condimentum libero.  Quisque porttitor, erat eget gravida feugiat, odio purus congue dui, nec varius purus turpis eget urna. Fusce facilisis est libero. Proin vitae libero vitae urna laoreet vulputate. Duis commodo, quam congue sodales posuere, neque ligula rhoncus nulla, cursus tristique neque ante et nunc. Donec placerat suscipit nulla vel faucibus. Vestibulum vehicula id diam eget feugiat. Donec vel diam fermentum, rutrum lectus id, vulputate dui.  Donec turpis risus, suscipit eu risus at, commodo suscipit massa. Quisque vel cursus leo, vitae tincidunt lacus. Vivamus fermentum tristique leo, vitae tempus diam faucibus eu. Nullam condimentum, est vitae vehicula facilisis, risus nulla viverra magna, quis elementum nunc nunc id mauris. Aliquam ante urna, volutpat accumsan lectus sit amet, scelerisque tristique orci. Sed sodales commodo purus ac ultrices. Mauris imperdiet ullamcorper luctus. Mauris faucibus metus a turpis blandit placerat. Donec interdum sem vitae quam convallis euismod.  Donec a magna elit. Vestibulum ante ipsum primis in faucibus orci luctus et ultrices posuere cubilia Curae; Ut blandit nisi augue, non porttitor dolor fringilla quis. Donec placerat a odio quis fringilla. Cras vitae aliquet nisl. Sed consequat dolor massa, et vulputate nibh dignissim eu. Donec dignissim cursus risus vel rutrum. Aliquam a faucibus nulla, sit amet blandit justo. Pellentesque id tortor sagittis, suscipit diam sed, imperdiet augue. Integer sit amet sem ac velit fermentum pharetra id a erat. In iaculis enim nec malesuada blandit.  Aenean malesuada sem non felis bibendum, blandit rhoncus turpis faucibus. Nam interdum massa dolor. Phasellus scelerisque rhoncus orci. Nullam hendrerit leo eget sem rutrum, viverra ultricies tortor congue. Suspendisse venenatis, augue id scelerisque molestie, dui arcu vestibulum eros, vitae facilisis augue massa at lectus. Maecenas at pulvinar magna. Suspendisse consequat diam vel augue molestie vehicula. Class aptent taciti sociosqu ad litora torquent per conubia nostra, per inceptos himenaeos. Vivamus ac commodo eros. Donec sit amet magna eget nibh dictum congue. Cras sapien odio, aliquam quis ullamcorper ut, interdum sed lectus. Aliquam risus justo, pellentesque vel magna in, fringilla porttitor magna. Pellentesque eleifend a augue nec rutrum. Nullam et lectus eu diam placerat semper. Pellentesque eget aliquam dui.  Nulla ultrices neque tincidunt, adipiscing leo eget, auctor augue. Sed ac metus convallis, consectetur eros eu, adipiscing lacus. Sed pellentesque ac sem nec tristique. Mauris imperdiet orci id nisl ullamcorper, non euismod erat tincidunt. Duis blandit facilisis dignissim. Quisque at tempus ligula. Cum sociis natoque penatibus et magnis dis parturient montes, nascetur ridiculus mus. Nullam tincidunt nibh felis, ut congue ligula lacinia nec. Sed ut ipsum vel elit tristique laoreet quis in diam. Etiam tempor erat eu aliquam tristique. Pellentesque habitant morbi tristique senectus et netus et malesuada fames ac turpis egestas. Nulla facilisi.  Maecenas cursus elit at varius lacinia. Etiam feugiat arcu sodales felis feugiat, et lobortis quam varius. Fusce et libero vitae dolor tincidunt tempor id ac lectus. Nam mollis viverra cursus. Nullam ut commodo mi, sit amet pretium lorem. Etiam tempus, velit sit amet lacinia lobortis, metus tellus vulputate orci, eu adipiscing metus dui et mauris. Nunc egestas consectetur nisi ut porta. Donec nec vehicula ligula. Nulla volutpat mi ac ornare elementum. Nullam risus justo, fringilla id tincidunt sit amet, elementum at purus. Cras a ullamcorper tellus, ac congue mi. Etiam malesuada leo a dui convallis pulvinar. Cum sociis natoque penatibus et magnis dis parturient montes, nascetur ridiculus mus. Morbi at ullamcorper nulla.  Curabitur eu molestie orci, porttitor feugiat quam. Pellentesque neque turpis, ullamcorper adipiscing scelerisque a, facilisis quis magna. Quisque nulla elit, luctus eget scelerisque non, scelerisque quis massa. Ut porttitor ante at mauris scelerisque adipiscing. Integer vel leo magna. Phasellus quam enim, malesuada et dignissim a, tempus id lorem. Nullam mattis tincidunt venenatis. Sed quam arcu, molestie sed ante vel, pulvinar fermentum mi. Nam malesuada id nibh sit amet dictum. Aliquam mi augue, mattis sit amet congue sed, dignissim ut odio.  Mauris scelerisque libero eget metus venenatis, ut mollis eros consectetur. Duis metus augue, molestie eget tincidunt vitae, volutpat vel lacus. Mauris fringilla imperdiet fermentum. Sed sit amet diam ut risus vulputate feugiat. Nulla vitae adipiscing quam. Duis non libero urna. Aenean ut ligula sed erat dictum dignissim aliquet non libero. Praesent quis neque varius lorem porta pulvinar. Integer aliquet elit vitae pretium mattis. Ut egestas nunc quis molestie commodo. Cras augue quam, cursus tristique sollicitudin sed, sagittis non velit. Lorem ipsum dolor sit amet, consectetur adipiscing elit. Donec malesuada in enim sed aliquam.  Curabitur lobortis fermentum purus. Maecenas vitae nibh ut libero congue interdum. Donec viverra ligula quis nibh volutpat, non luctus est dignissim. Suspendisse molestie, enim tempor consectetur gravida, ante sem porta mauris, a blandit velit quam suscipit justo. Etiam placerat euismod enim a rutrum. Praesent a imperdiet urna. Morbi quis vehicula leo. Nullam dictum fermentum nulla.  Mauris blandit pretium ultricies. Morbi ultrices est non sem suscipit mollis. Nam consequat ac ligula nec commodo. Ut mattis, tortor in laoreet tristique, quam dolor ornare massa, non luctus lacus ante eu massa. Nulla facilisi. Aliquam fringilla, felis non faucibus tempor, lorem sapien imperdiet mauris, rhoncus fermentum tellus nibh ut purus. Sed luctus risus quis mi interdum mollis. Duis sit amet nibh vel sem tincidunt vestibulum sed non eros. Duis laoreet orci dignissim est luctus, et pellentesque felis pulvinar.  Nam interdum massa eros, eu fringilla augue condimentum quis. Vestibulum pharetra mi quis felis hendrerit, eget malesuada nisl sagittis. Aliquam sit amet urna eu mauris dictum pharetra. Sed dignissim dignissim metus et elementum. Maecenas gravida lobortis tincidunt. Nulla dignissim, risus eu aliquam eleifend, lacus mi lobortis neque, sed venenatis erat ante at purus. Aliquam erat volutpat.  Nam eu eros a nisi mollis pretium vel vitae massa. Donec vulputate, ligula at fringilla ultrices, purus metus vehicula risus, ac sagittis purus metus sit amet libero. Curabitur eu dapibus urna, sed pharetra mauris. Mauris eget lacinia libero, vitae turpis duis."
        }


def gen_num_docs(size_doc):
    if size_doc == "small":
        # 335.000 fits in memory for the table with the small cache
        # 21.000.000 fits in memory for the table with the big cache
        return 1000000
    else:
        # 1000 fits in memory for the table with the small cache
        # 58000 fits in memory for the table with the big cache
        return 30000

def compare(new_results, previous_results):
    str_date = time.strftime("%y.%m.%d-%H:%M:%S")
    
    if not os.path.exists('comparisons'):
        os.mkdir('comparisons')
    elif not os.path.isdir('comparisons'):
        raise Exception('Unable to write the results as there is a non-folder named "comparisons"')
    
    f = open("comparisons/comparison_" + str_date + ".html", "w")
    f.write('''<html>
<head>
	<style>
		table {padding: 0px; margin: 0px;border-collapse:collapse;}
		th {cursor: hand}
		td, th {border: 1px solid #000; padding: 5px 8px; margin: 0px; text-align: right;}
	</style>
	<script type='text/javascript' src='jquery-latest.js'></script>
	<script type='text/javascript' src='jquery.tablesorter.js'></script>
	<script type='text/javascript' src='main.js'></script>
</head>
<body>
	%(previous_hash)s
	Current hash: %(current_hash)s</br>
	</br>
	<table>
		<thead><tr>
			<th>Query</th>
			<th>Previous avg q/s</th>
			<th>Avg q/s</th>
			<th>Previous 1st centile q/s</th>
			<th>1st centile q/s</th>
			<th>Previous 99 centile q/s</th>
			<th>99 centile q/s</th>
			<th>Diff</th>
			<th>Status</th>
		</tr></thead>
		<tbody>
''' % {
        'previous_hash': "Previous hash: " + previous_results["hash"] + "<br/>" if "hash" in previous_results else '',
        'current_hash': new_results["hash"]
    })

    for key in new_results:
        if key != "hash":
            
            reportValues = {
                'key50': str(key)[:50], 'status_color':'gray', 'status': 'Unknown', 'diff': 'undefined',
                'inverse_prev_average': 'Unknown',       'inverse_new_average': "%.2f" % (1 / new_results[key]["average"]),
                'inverse_prev_first_centile': 'Unknown', 'inverse_new_first_centile': "%.2f" % (1 / new_results[key]["first_centile"]),
                'inverse_prev_last_centile': 'Unknown',  'inverse_new_last_centile': "%.2f" % (1 / new_results[key]["last_centile"])
            }
            
            if key in previous_results:
                if new_results[key]["average"] > 0:
                    reportValues['diff'] = 1.0 * (1 / previous_results[key]["average"] - 1 / new_results[key]["average"]) / (1 / new_results[key]["average"])
                    print(reportValues['diff'], type(reportValues['diff']))
                    if type(reportValues['diff']) == type(0.):
                        if(reportValues['diff'] < 0.2):
                            reportValues['status'] = "Success"
                            reportValues['status_color'] = "green"
                        else:
                            reportValues['status'] = "Fail"
                            reportValues['status_color'] = "red"
                        reportValues['diff'] = "%.4f" % reportValues['diff']
                    else:
                        reportValues['status'] = "Bug"
                    
                    reportValues['inverse_prev_average'] = "%.2f" % (1 / previous_results[key]["average"])
                    reportValues['inverse_prev_first_centile'] =  "%.2f" % (1 / previous_results[key]["first_centile"])
                    reportValues['inverse_prev_last_centile'] = "%.2f" % (1 / previous_results[key]["last_centile"])
            
            try:
                f.write('''			<tr>
				<td>%(key50)s</td>
				<td>%(inverse_prev_average)s</td>
				<td>%(inverse_new_average)s</td>
				<td>%(inverse_prev_first_centile)s</td>
				<td>%(inverse_new_first_centile)s</td>
				<td>%(inverse_prev_last_centile)s</td>
				<td>%(inverse_new_last_centile)s</td>
				<td>%(diff)s</td>
				<td style='background: %(status_color)s'>%(status)s</td>
			</tr>
''' % reportValues)
            except Exception as e:
                print(key, str(e))

    f.write('''		</tbody>
	</table>
</body>
</html>
''')
    f.close()

    print("HTML file saved in comparisons/comparison_" + str_date + ".html")
