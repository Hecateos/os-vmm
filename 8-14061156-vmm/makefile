vmm:	vmm.c vmm.h request
	cc -o vmm vmm.c vmm.h
request:do_request.c vmm.h
	cc -o request do_request.c vmm.h
clean:
	rm vmm request
