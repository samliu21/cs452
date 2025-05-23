.global create
.global my_tid
.global my_parent_tid
.global yield
.global exit
.global syscall_exit
.global send
.global receive
.global reply
.global await_event
.global cpu_usage
.global terminate

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
syscall_exit:
    svc #4
    ret
send:
    svc #5
    ret

receive:
    svc #6
    ret

reply:
    svc #7
    ret

await_event:
    svc #8
    ret

cpu_usage:
	svc #9
	ret

terminate:
	svc #10
	ret
	