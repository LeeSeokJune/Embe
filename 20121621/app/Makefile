APP	:= app

all: app

app:
	arm-none-linux-gnueabi-gcc -static -o $(APP) $(APP).c

install:
	cp -a $(APP) /nfsroot
clean:
	rm -rf $(APP)
