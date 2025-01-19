.global create
.global my_tid
.global my_parent_tid
.global yield
.global exit

create:
	svc #0
	ret
my_tid:
    svc #1
    ret
my_parent_tid:
	svc #2
	ret
yield:
	svc #3
	ret
exit:
	svc #4
	ret
