if exist build rd /s /q build
md build
cd build
lupdate -no-obsolete "%~dp0..\src\dd.pro"
lrelease -nounfinished -removeidentical "%~dp0..\src\dd.pro"
cd ..
rd /s /q build
