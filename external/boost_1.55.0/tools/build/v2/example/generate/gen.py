
from b2.build.virtual_target import NonScanningAction, FileTarget

def generate_example(project, name, ps, sources):

    result = []
    for s in sources:

        a = NonScanningAction([s], "common.copy", ps)

        # Create a target to represent the action result. Uses the target name
        # passed here via the 'name' parameter and the same type and project as
        # the source.
        result.append(FileTarget(name, s.type(), project, a))

    return result
