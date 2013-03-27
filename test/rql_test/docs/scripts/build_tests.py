import os
import sys
import yaml

# tree of YAML documents defining documentation
src_dir = sys.argv[1]

commands = []

# Walk the src files to compile all sections and commands
for root, dirs, file_names in os.walk(src_dir):
    for file_name in file_names:
        docs = yaml.load(file(os.path.join(root, file_name)))

        if 'commands' in docs:
            commands.extend(docs['commands'])

def build_script(lang):
    with open('build/test.'+lang, 'w+') as out:
        with open('../drivers/driver.'+lang) as driver:
            out.write(driver.read())
            out.write('\n')

            for command in commands:
                section_name = command['section']
                command_name = command['tag']

                for i,example in enumerate(command['examples']):
                    test_tag = section_name+"-"+command_name+"-"+str(i)
                    test_case = example['code']
                    if isinstance(test_case, dict):
                        if lang in test_case:
                            test_case = test_case[lang]
                        else:
                            test_case = None

                    # Check for an override of this test case
                    if lang in command and isinstance(command[lang], dict):
                        override = command[lang]
                        if 'examples' in override:
                            if str(i) in override['examples']:
                                example_override = override['examples'][str(i)]

                                if len(example_override) == 0:
                                    test_case = None
                                elif 'code' in example_override:
                                    test_case = example_override['code']

                    if test_case != None:
                        out.write("test("+repr(test_case)+", '', "+repr(test_tag)+")\n")

build_script('py')
build_script('js')
build_script('py')
