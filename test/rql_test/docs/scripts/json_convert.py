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

        if 'sections' in docs:
            sections.extend(docs['sections'])
        if 'commands' in docs:
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
        'order': section['order'] if 'order' in section else 999,
        'commands':[]
    }

    commands_for_this_section = []
    for i,command in enumerate(commands):
        if command['section'] == section['tag']:
            def or_default(attr, default):
                if attr in command:
                    return command[attr]
                else:
                    return default

            out_command = {
                'tag':or_default('tag', ''),
                'description':or_default('description', ''),
                'parent':or_default('parent', ''),
                'returns':or_default('returns', ''),
                'order':or_default('order', i),
                'langs': {}
            }

            for lang in ['py', 'js', 'rb']:
                def or_override(obj, attr, default):
                    if not attr in obj:
                        # Use default
                        return default
                    else:
                        prop = obj[attr]
                        if not isinstance(prop, dict):
                            # Use given prop value
                            return prop
                        elif lang in prop:
                            # Use language specific override
                            return prop[lang]
                        else:
                            return default

                out_lang = {
                    'name':or_override(command, 'name', command['tag']),
                    'body':or_override(command, 'body', ''),
                    'dont_need_parenthesis':or_override(command, 'dont_need_parenthesis', False),
                    'examples': []
                }

                if 'is_operator' in command:
                    out_lang['is_operator'] = command['is_operator']
                if 'is_selector' in command:
                    out_lang['is_selector'] = command['is_selector']
                if 'name1' in command:
                    out_lang['name1'] = command['name1']
                if 'name2' in command:
                    out_lang['name2'] = command['name2']

                out_examples = []
                if 'examples' in command:
                    for example in command['examples']:
                        out_example = {
                            'code':or_override(example, 'code', ''),
                            'can_try':or_override(example, 'can_try', False),
                            'dataset':or_override(example, 'dataset', None),
                            'description':or_override(example, 'description', '')
                        }

                        out_examples.append(out_example)

                # Now process individual language overrides
                if lang in command:
                    override = command[lang]

                    if override == False:
                        out_lang = {'examples':[]}
                    else:
                        if 'name' in override:
                            out_lang['name'] = override['name']
                        if 'body' in override:
                            out_lang['body'] = override['body']
                        if 'dont_need_parenthesis' in override:
                            out_lang['dont_need_parenthesis'] = override['dont_need_parenthesis']
                        if 'is_operator' in override:
                            out_lang['is_operator'] = override['is_operator']
                        if 'is_selector' in override:
                            out_lang['is_selector'] = override['is_selector']
                        if 'name1' in override:
                            out_lang['name1'] = override['name1']
                        if 'name2' in override:
                            out_lang['name2'] = override['name2']

                        if 'examples' in override:
                            for example_num, example_override in override['examples'].iteritems():
                                if len(example_override) == 0:
                                    del out_examples[int(example_num)]
                                else:
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
    out_section['commands'].sort(key=lambda command: command['order'])
    out_obj['sections'].append(out_section)

out_obj['sections'].sort(key=lambda section: section['order'])

# And add header information
out_obj = dict(json.load(open("header.json", "r")).items() + out_obj.items())

# Serialize and write the output
out_file = file(dest_file, 'w')
json.dump(out_obj, out_file)
