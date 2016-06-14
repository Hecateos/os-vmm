vmm: vmm.c vmm.h request error.c
	cc -o vmm vmm.c vmm.h error.c
request:do_request.c vmm.h error.c
	cc -o request do_request.c vmm.h error.c
clean:
	rm vmm
