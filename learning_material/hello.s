.global quick_sort
.type quick_sort, %function
quick_sort:
    stp x29, x30, [sp, -32]! // save fp and lr on stack
    mov x29, sp
    // x0 arr, w1 low, w2 high
    cmp w1, w2
    bge done_quick_sort
    str x0, [sp, 32] // save x0 to sp+32
    str w1, [sp, 24] // save w1=low to sp+24
    str w2, [sp, 20] // save w2=high to sp+20
    bl partition //pi will overwrite x0/w0
    mov w7, w0 // save pi to w7
    ldr x0, [sp, 32] //restore x0 arr
    ldr w1, [sp, 24] //w1=low
    sub w2, w7, #1 //w2=pi-1
    bl quick_sort // arr,low,pi-1
    ldr x0, [sp, 32] //restore x0
    add w1, w7, #1 //w1=pi+1
    ldr w2, [sp, 20] //w2=high
    bl quick_sort//arr,pi+1,high
done_quick_sort:
    ldp x29, x30, [sp], #32
    ret

partition:
    // x0 arr, w1 low, w2 high
    add x4, x0, w2, UXTW #2 // x4=&arr[high]= arr+high<<2=arr+high*4, because int is 4 byte per element
    ldr w3, [x4]//w3=pivot=arr[high]
    sub w9, w1, #1 // w9=i=low-1
    mov w10, w1 //w10=j=low
    sub w11,w2,#1 //w11=high-1
partition_loop: //the for loop
    cmp w10,w11
    bgt done_partition_loop
    ldr w12,[x0,w10,UXTW #2] //w12=arr[j]
    cmp w12,w3 //compare arr[j],pivot
    bge end_swap_if //stop if arr[j]>=pivot
    add w9,w9,#1 //i++
    ldr w13, [x0, w9, UXTW #2] // w13=arr[i]
    str w13, [x0,w10, UXTW #2] // arr[i]->arr[j]
    str w12,[x0,w9,UXTW #2] //arr[j]->arr[i]

end_swap_if:
    add w10,w10,#1 //j++
    b partition_loop //continue for loop

done_partition_loop:
    add w9, w9, #1 // i=i+1
    ldr w14, [x0, w9, UXTW #2] // w14=arr[i+1]
    ldr w15, [x0, w2, UXTW #2] // w15=arr[high]
    str w14, [x0, w2, UXTW #2] // arr[high]=arr[i+1]
    str w15, [x0, w9, UXTW #2] // arr[i+1]=arr[high]
    mov w0, w9 // mov i+1 to return
    ret

.global memcpy_asm
.type memcpy_asm, %function
memcpy_asm: // w0 = d, w1 = s, w2 = n
    mov x3, x0 // w3 = w0 = d
    mov x4, x1 // w4 =  w1 = s
    b plus_copy_condition
plus_copy:
    ldr w6, [x4]
    str w6, [x3]
    sub w2, w2, #4 //--n
    add x3, x3, #4 //++d,因为是对指针做操作,指针就是地址,一定是64位,所以是x
    add x4, x4, #4 //++s
    ldr w6, [x4]
    str w6, [x3]
plus_copy_condition:  // do while is better than while
    cmp w2, 0
    bgt plus_copy
fin_memcpy:
    ret

.global sum_asm
.type sum_asm, %function
sum_asm:
    mov w1, w0
    mov w2, #0
loop_sum:
    cmp w1, #0
    blt fin_sum
    add w2, w1, w2
    sub w1, w1, #1
    b loop_sum

fin_sum:
    mov w0, w2
    ret

.global abs_asm          // 声明 abs 函数为全局可见
.type abs_asm, %function // 设置 abs 为函数类型

abs_asm:
    cmp w0, #0
    bge .jmp
    neg w0, w0

.jmp:
    ret
