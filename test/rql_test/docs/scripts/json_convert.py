import json
import yaml
import sys
import os

# tree of YAML documents defining documentation
src_dir = sys.argv[1]

# JSON output file of the old format
dest_file = sys.argv[2]

# Merged list of all sections defined in the input files
sections = []
# Merged list of all commands defined in the input files
commands = []

# Walk the src files to compile all sections and commands
for root, dirs, file_names in os.walk(src_dir):
    for file_name in file_names:
        docs = yaml.load(file(os.path.join(root, file_name)))

        sections.extend(docs['sections'])
        commands.extend(docs['commands'])

## Convert the input format to the output format
# This involves several steps
#  Shuffle the commands into their sections
#  Push inherited properties downwards
#  Fill in default values
out_obj = {'sections':[]}

for section in sections:
    out_section = {
        'name':section['name'],
        'tag':section['tag'],
        'description':section['description'],
        'commands':[]
    }

    commands_for_this_section = []
    for command in commands:
        if command['section'] == section['tag']:

            out_command = {
                'tag':command['tag'],
                'description':command['description'],
                'parent':command['parent'],
                'returns':command['returns'],
                'langs': {}
            }

            for lang in ['py', 'js', 'rb']:
                out_lang = {
                    'name':command['tag'],
                    'body':'',
                    'dont_need_parenthesis':False,
                    'examples': []
                }

                if 'name' in command:
                    out_lang['name'] = command['name']
                if 'body' in command:
                    out_lang['body'] = command['body']
                if 'dont_need_parenthesis' in command:
                    out_lang['dont_need_parenthesis'] = command['dont_need_parenthesis']

                out_examples = []
                for example in command['examples']:
                    out_example = {
                        'code':example['code'],
                        'can_try': False,
                        'dataset': '',
                        'description':example['description']
                    }

                    if 'can_try' in example:
                        out_example['can_try'] = example['can_try']
                    if 'dataset' in example:
                        out_example['dataset'] = example['dataset']

                    out_examples.append(out_example)

                # Now process individual language overrides 
                if lang in command:
                    for override in command[lang]:
                        if 'name' in override:
                            out_lang['name'] = override['name']
                        if 'body' in override:
                            out_lang['body'] = override['body']
                        if 'dont_need_parenthesis' in override:
                            out_lang['dont_need_parenthesis'] = override['dont_need_parenthesis']

                        for example_num, example_override in override['examples'].iteritems():
                            if 'code' in example_override:
                                out_examples[int(example_num)]['code'] = example_override['code']
                            if 'can_try' in example_override:
                                out_examples[int(example_num)]['can_try'] = example_override['can_try']
                            if 'dataset' in example_override:
                                out_examples[int(example_num)]['dataset'] = example_override['dataset']
                            if 'description' in example_override:
                                out_examples[int(example_num)]['description'] = example_override['description']

                out_lang['examples'].extend(out_examples)
                out_command['langs'][lang] = out_lang

            commands_for_this_section.append(out_command)

    out_section['commands'].extend(commands_for_this_section)
    out_obj['sections'].append(out_section)

# Serialize and write the output
out_file = file(dest_file, 'w')
json.dump(out_obj, out_file)
