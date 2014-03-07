// exception: Unable to identify anything without a valid scope manager
context.setCode('42');

// manual hijacking
context._scopeManager = null;

context.identify(1);
