#!/usr/bin/python
# -*- coding: utf-8 -*-

'''Create the dmg for MacOS deployment'''

import atexit, copy, glob, os, re, shutil, subprocess, sys, tempfile

thisFolder = os.path.dirname(os.path.realpath(__file__))
sys.path.append(thisFolder)
import dmgbuild

# == defaults

scratchFolder = tempfile.mkdtemp()

packagePosition = (470, 170)

defaultOptions = {
	'format': 'UDBZ',
	'badge_icon': os.path.join(thisFolder, 'Thinker.icns'),
	'files': [
		os.path.join(thisFolder, os.path.pardir, os.path.pardir, 'COPYRIGHT')
	],
	'icon_size': 64.0,
	'text_size': 14.0,
	'icon_locations': {
		'Uninstall RethinkDB.app': (630, 170),
		'Release Notes.url': (470, 303),
		'COPYRIGHT': (630, 303)
	},
	'background': os.path.join(thisFolder, 'dmg_background.png'),
	'window_rect': ((200, 200), (739, 420)),
	'default_view': 'icon-view',
	'show_icon_preview': True
}

# ==

def removeAtExit(removePath):
	if os.path.exists(removePath):
		try:
			if os.path.isfile(removePath):
				os.unlink(removePath)
			elif os.path.isdir(removePath):
				shutil.rmtree(removePath, ignore_errors=True)
		except Exception as e:
			sys.stderr.write('Unable to delete item: %s -- %s\n' % (removePath, str(e)))
atexit.register(removeAtExit, scratchFolder)

def compileUninstallApp():
	outputPath = os.path.join(scratchFolder, 'Uninstall RethinkDB.app')
	
	# - compile the app
	
	logFile = open(os.path.join(scratchFolder, 'uninstall-compile.log'), 'w+')
	try:
		subprocess.check_call(['/usr/bin/osacompile', '-o', outputPath, os.path.join(thisFolder, 'uninstall.scpt')], stdout=logFile, stderr=logFile)
	except Exception as e:
		logFile.seek(0)
		sys.stderr.write('Failed while compiling %s: %s\n%s' % (os.path.join(thisFolder, 'uninstall.scpt'), str(e), logFile.read()))
		raise
	
	# - change the icon - ToDo: re-do this once an icon is chosen
	
	#iconPath = os.path.join(outputPath, 'Contents', 'Resources', 'applet.icns')
	#os.unlink(iconPath)
	#os.symlink('/System/Library/CoreServices/CoreTypes.bundle/Contents/Resources/Unsupported.icns', iconPath)
	
	# -
	
	return outputPath

def makeReleaseNotesLink(version):
	notesPath = os.path.join(scratchFolder, 'Release Notes.url')
	with open(notesPath, 'w') as outputFile:
		outputFile.write('[InternetShortcut]\nURL=https://github.com/rethinkdb/rethinkdb/releases/tag/v%s\n' % version)
	return notesPath

def buildPackage(versionString, serverRootPath, installPath=None, signingName=None):
	'''Generate a .pkg with all of our customizations'''
	
	# == check for the identity
	
	if signingName is not None:
		signingName = str(signingName)
		for line in subprocess.check_output(['/usr/bin/security', 'find-identity', '-p', 'macappstore', '-v']).splitlines():
			if signingName in line:
				break
		else:
			raise ValueError('Could not find the requested signingName: %s' % signingName)
	
	# == build the component packages
	
	# - make the server package
	
	packageFolder = os.path.join(scratchFolder, 'packages')
	if not os.path.isdir(packageFolder):
		os.mkdir(packageFolder)
	
	serverPackagePath = os.path.join(packageFolder, 'rethinkdb_server.pkg')
	logFile = open(os.path.join(scratchFolder, 'rethinkdb_server_pkg.log'), 'w+')
	
	pkgCommand = ['/usr/bin/pkgbuild', serverPackagePath, '--root', serverRootPath, '--identifier', 'com.rethinkdb.server', '--version', versionString]
	if installPath:
	   pkgCommand += ['--install-location', str(installPath)]
	
	try:
		subprocess.check_call(pkgCommand, stdout=logFile, stderr=logFile)
	except Exception as e:
		logFile.seek(0)
		sys.stderr.write('Failed while building server package: %s\n%s' % (str(e), logFile.read()))
		raise
	
	# - installer_resources
	
	installerResourcesPath = os.path.join(scratchFolder, 'installer_resources')
	os.mkdir(installerResourcesPath)
	subprocess.check_call(
	   ['/usr/bin/tiffutil',
	   '-cathidpicheck'] + glob.glob(os.path.join(thisFolder, 'installer_resources', 'installer_background*.png')) +
	   ['-out', os.path.join(installerResourcesPath, 'installer_background.tiff')]
    )
	
	# == assemble the archive
	
	distributionPath = os.path.join(scratchFolder, 'rethinkdb-%s.pkg' % versionString)
	
	productBuildCommand = [
	   '/usr/bin/productbuild',
	   '--distribution', os.path.join(thisFolder, 'Distribution.xml'),
	   '--package-path', packageFolder,
	   '--resources', installerResourcesPath,
	   distributionPath
    ]
	if signingName is not None:
		productBuildCommand += ['--sign', signingName]
	
	logFile = open(os.path.join(scratchFolder, 'rethinkdb_pkg.log'), 'w+')
	try:
		subprocess.check_call(productBuildCommand, stdout=logFile, stderr=logFile)
	except Exception as e:
		logFile.seek(0)
		sys.stderr.write('Failed while compiling %s: %s\n%s' % (os.path.join(thisFolder, 'uninstall.scpt'), str(e), logFile.read()))
		raise
	return distributionPath

def main():
	'''Parse command line input and run'''
	
	global scratchFolder
	
	# == process input
	
	import optparse
	parser = optparse.OptionParser()
	parser.add_option('-s', '--server-root',     dest='serverRoot',    default=None,        help='path to the server component')
	parser.add_option('-i', '--install-path',    dest='installPath',    default=None,       help='path to install to')
	parser.add_option('-o', '--ouptut-location', dest='outputPath',                         help='location for the output file')
	parser.add_option(      '--rethinkdb-name',  dest='binaryName',    default='rethinkdb', help='name of the rethinkdb server binary')
	parser.add_option(      '--signing-name',    dest='signingName',   default=None,        help='signing identifier')
	parser.add_option(      '--scratch-folder',  dest='scratchFolder', default=None,        help='folder for intermediate products')
	options, args = parser.parse_args()
	
	if len(args) > 0:
		parser.error('no non-keyword options are allowed')
	
	# = -s/--server-root 
	
	if options.serverRoot is None:
		parser.error('-s/--server-root is required')
	if not os.path.isdir(options.serverRoot):
		parser.error('-s/--server-root must be a folder: %s' % options.serverRoot)
	options.serverRoot = os.path.realpath(options.serverRoot)
	
	# = get the version of rethinkdb
	
	# find the binary
	
	rethinkdbPath = os.path.join(options.serverRoot, options.binaryName)
	if not os.access(rethinkdbPath, os.X_OK):
		parser.error('No runnable RethinkDB executable at: %s' % rethinkdbPath)
	
	# get the version string
	
	versionString = ''
	try:
		versionString = subprocess.check_output([rethinkdbPath, '--version']).strip().split()[1].decode('utf-8')
	except Exception as e:
		print(e)
		parser.error('the executable given is not a valid RethinkDB executable: %s' % rethinkdbPath)
	
	strictVersion = re.match('^(\d+\.?)+(-\d+)?', versionString)
	if strictVersion is None:
		parser.error('version string from executable does not have a regular version string: %s' % versionString)
	strictVersion = strictVersion.group()
	
	# = -o/--ouptut-location
	
	if options.outputPath is None:
		options.outputPath = os.path.join(thisFolder, 'RethinkDB ' + versionString + '.dmg')
	elif os.path.isdir(options.outputPath):
		options.outputPath = os.path.join(options.outputPath, 'RethinkDB ' + versionString + '.dmg')
	elif not os.path.isdir(os.path.dirname(options.outputPath)):
		parser.error('the output path given is not valid: %s' % options.outputPath)
	
	# = --scratch-folder
	
	if options.scratchFolder is not None:
		if not os.path.isdir(options.scratchFolder):
			parser.error('the --scratch-folder given is not an existing folder: %s' % options.scratchFolder)
		scratchFolder = options.scratchFolder
	
	# == build the pkg	
	
	pkgPath = buildPackage(versionString, options.serverRoot, installPath=options.installPath, signingName=options.signingName)
	
	# == add dynamic content to settings
	
	dmgOptions = copy.deepcopy(defaultOptions)
	
	# = package
	
	dmgOptions['files'].append(pkgPath)
	dmgOptions['icon_locations'][os.path.basename(pkgPath)] = packagePosition
	
	# = uninstall script
	
	uninstallAppPath = compileUninstallApp()
	dmgOptions['files'].append(uninstallAppPath)
	
	# = release notes
	
	dmgOptions['files'].append(makeReleaseNotesLink(strictVersion))
	
	# == dmg creation
	
	dmgbuild.build_dmg(options.outputPath, 'RethinkDB ' + versionString, defines=dmgOptions)

if __name__ == '__main__':
	main()
