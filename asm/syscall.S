.global create
.global my_tid
.global my_parent_tid

create:
	svc #0
	ret

my_tid:
    svc #1
    ret

my_parent_tid:
	svc #2
	ret
