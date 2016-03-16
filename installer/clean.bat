@echo off
echo clean ui and moc files
Setlocal EnableDelayedExpansion
if exist resources\resource.cpp del resources\resource.cpp
if exist resources\files.7z del resources\files.7z