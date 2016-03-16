import os.path, time, sys, glob, re, shutil

def sign_file(file_name):
	temp_file_name = file_name + ".tmp"
	os.system("curl.exe -F file=@" + file_name + " http://sign.corp.mail.ru/sign2 -o " + temp_file_name)
	shutil.move(temp_file_name, file_name)

sign_file("installer.exe")
