import json
import yaml
import sys

src_file = sys.argv[1]
dest_file = sys.argv[2]

docs_obj = json.load(file(src_file))
out_file = file(dest_file, "w")
yaml.safe_dump(docs_obj, out_file)
