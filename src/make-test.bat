call "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvars64.bat"

cd unittest

msbuild -p:"Configuration=Debug" -p:"Platform=x64"
if %errorlevel% neq 0 exit /b %errorlevel%
x64\Debug\unittest.exe
if %errorlevel% neq 0 exit /b %errorlevel%

cd ..\test

msbuild -p:"Configuration=Debug" -p:"Platform=x64"
if %errorlevel% neq 0 exit /b %errorlevel%
@echo Testing Parser
x64\Debug\test.exe -m parse -i test_frontend.mini -o test-parse.out
if %errorlevel% neq 0 exit /b %errorlevel%
@echo Testing importing
x64\Debug\test.exe -m parse -i test_import1.mini -o test-import.out
if %errorlevel% neq 0 exit /b %errorlevel%
@echo Testing symantics
x64\Debug\test.exe -m semantic -i test_frontend.mini -o test-semantic.out
if %errorlevel% neq 0 exit /b %errorlevel%
@echo Testing codegen
x64\Debug\test.exe -m frontend -i test_frontend.mini -o test-frontend.out
if %errorlevel% neq 0 exit /b %errorlevel%

cd ..\mini
msbuild -p:"Configuration=Debug" -p:"Platform=x64"
@echo Testing Compiling
x64\Debug\mini.exe -c ..\test\ut-vm.mini
if %errorlevel% neq 0 exit /b %errorlevel%
@echo Testing VM
x64\Debug\mini.exe ..\test\ut-vm.mini
if %errorlevel% neq 0 exit /b %errorlevel%

@echo All tests are successful

cd ..