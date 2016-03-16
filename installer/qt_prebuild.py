if __name__ != '__main__':
	sys.exit(0)

import os.path, time, sys, glob, re, argparse

parser = argparse.ArgumentParser()
parser.add_argument("-config", type=str, help="build configuration (release or debug)")
args = parser.parse_args()

files_to_pack = [
	"icq.exe",
	"corelib.dll",
	"libvoip_x86.dll",
	"qresource"]


def get_7z_path():
	program_files = os.environ["ProgramFiles(x86)"]
	path_7z = program_files + "\\7-Zip\\7z.exe"
	if not os.path.exists(path_7z):
		program_files = os.environ["ProgramFiles"]
		path_7z = program_files + "\\7-Zip\\7z.exe"
				
	return path_7z
	
def pack_files():
	print("pack files:", "")
	sys.stdout.flush()
	path_7z = get_7z_path()
	if not os.path.exists(path_7z):
		print("pack files:", "7z not found")
		sys.exit(-1)

	archive_name = "resources/files.7z"
	
	if (not os.path.exists(archive_name) or args.config.lower() != "debug"):
		files_to_pack_str = ""	

		for file_to_pack in files_to_pack:
			file_name = "../bin/" + args.config + "/" + file_to_pack
			if not os.path.exists(file_name):
				print("pack files:", file_name + " not found")
				sys.exit(-1)

			files_to_pack_str += " " + file_name

		if os.path.exists(archive_name):
			os.remove(archive_name)

		os.system('"' + path_7z + '"' + " a -t7z -mx=9 -mf=off -r " + archive_name + files_to_pack_str)


def file_contains_regex(path, regexes):
	assert os.path.exists(path)
	assert len(regexes) > 0

	def test_line(line):
		for regex in regexes:
			if re.search(regex, line):
				return True

		return False

	def test_lines(file):
		for line in file:
			if test_line(line):
				return True

		return False

	with open(path) as file:
		return test_lines(file)

META_REGEXES = ("Q_(OBJECT|SLOTS|SIGNALS)",)

def file_contains_qmeta(path):
	return file_contains_regex(path, META_REGEXES)

def build_mocs_in(dir):
	assert dir[-1] != '/'
	assert dir[-1] != '\\'

	files = glob.glob(dir + "/*.h")
	for file in files:
		if not file_contains_regex(file, META_REGEXES):
			continue

		file_name = os.path.splitext(file)[0]
		h_file = os.path.abspath(file_name + ".h").replace("\\", "/")
		moc_file = os.path.abspath(dir + "/moc_" + os.path.basename(file_name) + ".cpp").replace("\\", "/")

		if os.path.exists(moc_file):
			if os.path.getmtime(h_file) > os.path.getmtime(moc_file):
				print(os.path.basename(h_file), "is newer that", os.path.basename(moc_file),">> rebuild")
				os.system("%QTPATH%/bin/moc.exe " + '"' + h_file + '"' + " -b stdafx.h" + " -o " + '"' + moc_file + '"')
		else:
			print("build", os.path.basename(moc_file))
			os.system("%QTPATH%/bin/moc.exe " + '"' + h_file + '"' + " -b stdafx.h" + " -o " + '"' + moc_file + '"')

	files = glob.glob(dir + "/moc_*.cpp")
	for file in files:
		file_size = os.path.getsize(file)
		MIN_SIZE = 3
		if file_size < MIN_SIZE:
			os.remove(file)

	dirs = glob.glob(dir + "/*/")
	for dir in dirs:
		build_mocs_in(dir[:-1])

#
# Main
#

translation_changed = False


files = glob.glob("translations\*ts")
for file in files:
	file_name = os.path.splitext(file)[0]
	ts_file = os.path.abspath(file_name + ".ts").replace("\\", "/")
	qm_file = os.path.abspath(file_name + ".qm").replace("\\", "/")
	if os.path.exists(qm_file):
		if os.path.getmtime(ts_file) > os.path.getmtime(qm_file):
			translation_changed = True
			print(os.path.basename(ts_file), "is newer that", os.path.basename(qm_file),">> rebuild")
			os.system("%QTPATH%/bin/lrelease.exe " + '"' + ts_file + '"')
	else:
		print("build", os.path.basename(qm_file))
		os.system("%QTPATH%/bin/lrelease.exe " + '"' + ts_file + '"')
		translation_changed = True


def compile_resources():
	qrc_file_name = os.path.abspath("resources/resource.qrc").replace("\\", "/")
	cpp_file_name = os.path.abspath("resources/resource.cpp").replace("\\", "/")
	qss_file_name = os.path.abspath("resources/styles/styles.qss").replace("\\", "/")
	
	if ((not os.path.exists(cpp_file_name)) or (os.path.getmtime(qrc_file_name) > os.path.getmtime(cpp_file_name)) or args.config.lower() != "debug" or (os.path.getmtime(qss_file_name) > os.path.getmtime(cpp_file_name))):
		os.system("%QTPATH%/bin/rcc.exe " + '"' + qrc_file_name + '"' + " -o " + '"' + cpp_file_name + '"')

pack_files()
build_mocs_in(".")
compile_resources()



sys.exit(0)