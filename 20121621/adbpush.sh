cd module/
make
adb push dev_driver.ko /data/local/tmp
cd ..

cd app/
make clean
make
adb push app /data/local/tmp
cd ..
