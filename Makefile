KROOT = /lib/modules/$(shell uname -r)/build

MODULES = dependencies/ driver/ 

obj-y += $(MODULES)

modules:
# 	@echo $(wildcard ./driver/*.c)
	@$(MAKE) -w -C $(KROOT) M=$(PWD) modules

modules_install:
	@$(MAKE) -C $(KROOT) M=$(PWD) modules_install

clean: 
	@$(MAKE) -C $(KROOT) M=$(PWD) clean
	rm -rf   Module.symvers modules.order

insert: modules
# 	sudo insmod $(addsuffix *.ko,$(wildcard $(MODULES)))
# 	@echo $(addsuffix *.ko,$(wildcard $(MODULES)))
# 	sudo insmod $(MODULE_NAME).ko
	sudo insmod driver/idt/idt_patcher.ko
	sudo insmod driver/haap.ko

remove:
	sudo rmmod haap
	sudo rmmod idt_patcher
# 	sudo rmmod $(patsubst %.ko,%,$(addsuffix *.ko,$(wildcard $(MODULES))))

# reboot: clean insert