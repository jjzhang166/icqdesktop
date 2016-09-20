from __future__ import print_function
if __name__ != '__main__':
	sys.exit(0)

import os, sys, shutil
import fileinput
import distutils.dir_util
import zipfile
import requests
from os.path import basename

def get_str_version_number(build_number):	
	ICQ_VERSION_MAJOR = '10'
	ICQ_VERSION_MINOR = '0'
	return ".".join((ICQ_VERSION_MAJOR, ICQ_VERSION_MINOR, build_number))
	
def zip_files(init_files, archive_name):
	archive = archive_name
	zf = zipfile.ZipFile(archive, mode='w')
	try:
		for file in init_files:
			zf.write(file, basename(file))
	except:
		return
	finally:
		zf.close
		return archive
	
def upload_install_to_hockey_app(version_id, name, files_to_send, archive_name, app_id, upload_only_token):
	url = 'https://rink.hockeyapp.net/api/2/apps/' + app_id + '/app_versions/' + str(version_id)
	
	archive = zip_files(files_to_send, archive_name)
	resp = requests.put(url, headers={'X-HockeyAppToken': upload_only_token}, files={name: open(archive, 'rb')})
	print(resp.status_code)

def publish_version_to_hockey_app(build_number, app_id, upload_only_token):
	url = 'https://rink.hockeyapp.net/api/2/apps/' + app_id + '/app_versions/new'
	resp = requests.post(url, data={'bundle_version': get_str_version_number(build_number)}, headers={'X-HockeyAppToken': upload_only_token})
	print(resp.status_code, resp.reason)
	data = resp.json()
	version_id = data['id']
	
	upload_folder = os.path.join(sys.argv[2], build_number).replace('\\', '/')
	setup = upload_folder + "/Release/installer/icqsetup.exe"
	pdb_installer = upload_folder + "/Release/installer/icqsetup.pdb"
	pdb_icq = upload_folder + "/Release/ICQ.pdb"
	pdb_corelib = upload_folder + "/Release/corelib.pdb"
	
	archive = upload_folder + "/Release/installer/dsym.zip"
	archive_pdb = upload_folder + "/Release/installer/dsym.zip"
	
	upload_install_to_hockey_app(version_id, "ipa", [setup], archive, app_id, upload_only_token)
	upload_install_to_hockey_app(version_id, "dsym", [pdb_installer, pdb_icq, pdb_corelib], archive_pdb, app_id, upload_only_token)

	
print("buildnumber=" + sys.argv[1])
print("storage=" + sys.argv[2])
print("app_id=" + sys.argv[3])
print("app_tocken=" + sys.argv[4])
print("installer_id=" + sys.argv[5])
print("installer_tocken=" + sys.argv[6])
print("publish app:")
publish_version_to_hockey_app(sys.argv[1], sys.argv[3], sys.argv[4])
print("publish installer:")
publish_version_to_hockey_app(sys.argv[1], sys.argv[5], sys.argv[6])

sys.exit(0)