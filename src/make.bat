call "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvars64.bat"

cd mini

msbuild -p:"Configuration=Release" -p:"Platform=x64"
if %errorlevel% neq 0 exit /b %errorlevel%

cd ..