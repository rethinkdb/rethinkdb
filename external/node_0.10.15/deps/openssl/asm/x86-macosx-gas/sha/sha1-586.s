.file	"sha1-586.s"
.text
.globl	_sha1_block_data_order
.align	4
_sha1_block_data_order:
L_sha1_block_data_order_begin:
	pushl	%ebp
	pushl	%ebx
	pushl	%esi
	pushl	%edi
	movl	20(%esp),%ebp
	movl	24(%esp),%esi
	movl	28(%esp),%eax
	subl	$76,%esp
	shll	$6,%eax
	addl	%esi,%eax
	movl	%eax,104(%esp)
	movl	16(%ebp),%edi
	jmp	L000loop
.align	4,0x90
L000loop:
	movl	(%esi),%eax
	movl	4(%esi),%ebx
	movl	8(%esi),%ecx
	movl	12(%esi),%edx
	bswap	%eax
	bswap	%ebx
	bswap	%ecx
	bswap	%edx
	movl	%eax,(%esp)
	movl	%ebx,4(%esp)
	movl	%ecx,8(%esp)
	movl	%edx,12(%esp)
	movl	16(%esi),%eax
	movl	20(%esi),%ebx
	movl	24(%esi),%ecx
	movl	28(%esi),%edx
	bswap	%eax
	bswap	%ebx
	bswap	%ecx
	bswap	%edx
	movl	%eax,16(%esp)
	movl	%ebx,20(%esp)
	movl	%ecx,24(%esp)
	movl	%edx,28(%esp)
	movl	32(%esi),%eax
	movl	36(%esi),%ebx
	movl	40(%esi),%ecx
	movl	44(%esi),%edx
	bswap	%eax
	bswap	%ebx
	bswap	%ecx
	bswap	%edx
	movl	%eax,32(%esp)
	movl	%ebx,36(%esp)
	movl	%ecx,40(%esp)
	movl	%edx,44(%esp)
	movl	48(%esi),%eax
	movl	52(%esi),%ebx
	movl	56(%esi),%ecx
	movl	60(%esi),%edx
	bswap	%eax
	bswap	%ebx
	bswap	%ecx
	bswap	%edx
	movl	%eax,48(%esp)
	movl	%ebx,52(%esp)
	movl	%ecx,56(%esp)
	movl	%edx,60(%esp)
	movl	%esi,100(%esp)
	movl	(%ebp),%eax
	movl	4(%ebp),%ebx
	movl	8(%ebp),%ecx
	movl	12(%ebp),%edx
	# 00_15 0

	movl	%ecx,%esi
	movl	%eax,%ebp
	roll	$5,%ebp
	xorl	%edx,%esi
	addl	%edi,%ebp
	movl	(%esp),%edi
	andl	%ebx,%esi
	rorl	$2,%ebx
	xorl	%edx,%esi
	leal	1518500249(%ebp,%edi,1),%ebp
	addl	%esi,%ebp
	# 00_15 1

	movl	%ebx,%edi
	movl	%ebp,%esi
	roll	$5,%ebp
	xorl	%ecx,%edi
	addl	%edx,%ebp
	movl	4(%esp),%edx
	andl	%eax,%edi
	rorl	$2,%eax
	xorl	%ecx,%edi
	leal	1518500249(%ebp,%edx,1),%ebp
	addl	%edi,%ebp
	# 00_15 2

	movl	%eax,%edx
	movl	%ebp,%edi
	roll	$5,%ebp
	xorl	%ebx,%edx
	addl	%ecx,%ebp
	movl	8(%esp),%ecx
	andl	%esi,%edx
	rorl	$2,%esi
	xorl	%ebx,%edx
	leal	1518500249(%ebp,%ecx,1),%ebp
	addl	%edx,%ebp
	# 00_15 3

	movl	%esi,%ecx
	movl	%ebp,%edx
	roll	$5,%ebp
	xorl	%eax,%ecx
	addl	%ebx,%ebp
	movl	12(%esp),%ebx
	andl	%edi,%ecx
	rorl	$2,%edi
	xorl	%eax,%ecx
	leal	1518500249(%ebp,%ebx,1),%ebp
	addl	%ecx,%ebp
	# 00_15 4

	movl	%edi,%ebx
	movl	%ebp,%ecx
	roll	$5,%ebp
	xorl	%esi,%ebx
	addl	%eax,%ebp
	movl	16(%esp),%eax
	andl	%edx,%ebx
	rorl	$2,%edx
	xorl	%esi,%ebx
	leal	1518500249(%ebp,%eax,1),%ebp
	addl	%ebx,%ebp
	# 00_15 5

	movl	%edx,%eax
	movl	%ebp,%ebx
	roll	$5,%ebp
	xorl	%edi,%eax
	addl	%esi,%ebp
	movl	20(%esp),%esi
	andl	%ecx,%eax
	rorl	$2,%ecx
	xorl	%edi,%eax
	leal	1518500249(%ebp,%esi,1),%ebp
	addl	%eax,%ebp
	# 00_15 6

	movl	%ecx,%esi
	movl	%ebp,%eax
	roll	$5,%ebp
	xorl	%edx,%esi
	addl	%edi,%ebp
	movl	24(%esp),%edi
	andl	%ebx,%esi
	rorl	$2,%ebx
	xorl	%edx,%esi
	leal	1518500249(%ebp,%edi,1),%ebp
	addl	%esi,%ebp
	# 00_15 7

	movl	%ebx,%edi
	movl	%ebp,%esi
	roll	$5,%ebp
	xorl	%ecx,%edi
	addl	%edx,%ebp
	movl	28(%esp),%edx
	andl	%eax,%edi
	rorl	$2,%eax
	xorl	%ecx,%edi
	leal	1518500249(%ebp,%edx,1),%ebp
	addl	%edi,%ebp
	# 00_15 8

	movl	%eax,%edx
	movl	%ebp,%edi
	roll	$5,%ebp
	xorl	%ebx,%edx
	addl	%ecx,%ebp
	movl	32(%esp),%ecx
	andl	%esi,%edx
	rorl	$2,%esi
	xorl	%ebx,%edx
	leal	1518500249(%ebp,%ecx,1),%ebp
	addl	%edx,%ebp
	# 00_15 9

	movl	%esi,%ecx
	movl	%ebp,%edx
	roll	$5,%ebp
	xorl	%eax,%ecx
	addl	%ebx,%ebp
	movl	36(%esp),%ebx
	andl	%edi,%ecx
	rorl	$2,%edi
	xorl	%eax,%ecx
	leal	1518500249(%ebp,%ebx,1),%ebp
	addl	%ecx,%ebp
	# 00_15 10

	movl	%edi,%ebx
	movl	%ebp,%ecx
	roll	$5,%ebp
	xorl	%esi,%ebx
	addl	%eax,%ebp
	movl	40(%esp),%eax
	andl	%edx,%ebx
	rorl	$2,%edx
	xorl	%esi,%ebx
	leal	1518500249(%ebp,%eax,1),%ebp
	addl	%ebx,%ebp
	# 00_15 11

	movl	%edx,%eax
	movl	%ebp,%ebx
	roll	$5,%ebp
	xorl	%edi,%eax
	addl	%esi,%ebp
	movl	44(%esp),%esi
	andl	%ecx,%eax
	rorl	$2,%ecx
	xorl	%edi,%eax
	leal	1518500249(%ebp,%esi,1),%ebp
	addl	%eax,%ebp
	# 00_15 12

	movl	%ecx,%esi
	movl	%ebp,%eax
	roll	$5,%ebp
	xorl	%edx,%esi
	addl	%edi,%ebp
	movl	48(%esp),%edi
	andl	%ebx,%esi
	rorl	$2,%ebx
	xorl	%edx,%esi
	leal	1518500249(%ebp,%edi,1),%ebp
	addl	%esi,%ebp
	# 00_15 13

	movl	%ebx,%edi
	movl	%ebp,%esi
	roll	$5,%ebp
	xorl	%ecx,%edi
	addl	%edx,%ebp
	movl	52(%esp),%edx
	andl	%eax,%edi
	rorl	$2,%eax
	xorl	%ecx,%edi
	leal	1518500249(%ebp,%edx,1),%ebp
	addl	%edi,%ebp
	# 00_15 14

	movl	%eax,%edx
	movl	%ebp,%edi
	roll	$5,%ebp
	xorl	%ebx,%edx
	addl	%ecx,%ebp
	movl	56(%esp),%ecx
	andl	%esi,%edx
	rorl	$2,%esi
	xorl	%ebx,%edx
	leal	1518500249(%ebp,%ecx,1),%ebp
	addl	%edx,%ebp
	# 00_15 15

	movl	%esi,%ecx
	movl	%ebp,%edx
	roll	$5,%ebp
	xorl	%eax,%ecx
	addl	%ebx,%ebp
	movl	60(%esp),%ebx
	andl	%edi,%ecx
	rorl	$2,%edi
	xorl	%eax,%ecx
	leal	1518500249(%ebp,%ebx,1),%ebp
	movl	(%esp),%ebx
	addl	%ebp,%ecx
	# 16_19 16

	movl	%edi,%ebp
	xorl	8(%esp),%ebx
	xorl	%esi,%ebp
	xorl	32(%esp),%ebx
	andl	%edx,%ebp
	xorl	52(%esp),%ebx
	roll	$1,%ebx
	xorl	%esi,%ebp
	addl	%ebp,%eax
	movl	%ecx,%ebp
	rorl	$2,%edx
	movl	%ebx,(%esp)
	roll	$5,%ebp
	leal	1518500249(%ebx,%eax,1),%ebx
	movl	4(%esp),%eax
	addl	%ebp,%ebx
	# 16_19 17

	movl	%edx,%ebp
	xorl	12(%esp),%eax
	xorl	%edi,%ebp
	xorl	36(%esp),%eax
	andl	%ecx,%ebp
	xorl	56(%esp),%eax
	roll	$1,%eax
	xorl	%edi,%ebp
	addl	%ebp,%esi
	movl	%ebx,%ebp
	rorl	$2,%ecx
	movl	%eax,4(%esp)
	roll	$5,%ebp
	leal	1518500249(%eax,%esi,1),%eax
	movl	8(%esp),%esi
	addl	%ebp,%eax
	# 16_19 18

	movl	%ecx,%ebp
	xorl	16(%esp),%esi
	xorl	%edx,%ebp
	xorl	40(%esp),%esi
	andl	%ebx,%ebp
	xorl	60(%esp),%esi
	roll	$1,%esi
	xorl	%edx,%ebp
	addl	%ebp,%edi
	movl	%eax,%ebp
	rorl	$2,%ebx
	movl	%esi,8(%esp)
	roll	$5,%ebp
	leal	1518500249(%esi,%edi,1),%esi
	movl	12(%esp),%edi
	addl	%ebp,%esi
	# 16_19 19

	movl	%ebx,%ebp
	xorl	20(%esp),%edi
	xorl	%ecx,%ebp
	xorl	44(%esp),%edi
	andl	%eax,%ebp
	xorl	(%esp),%edi
	roll	$1,%edi
	xorl	%ecx,%ebp
	addl	%ebp,%edx
	movl	%esi,%ebp
	rorl	$2,%eax
	movl	%edi,12(%esp)
	roll	$5,%ebp
	leal	1518500249(%edi,%edx,1),%edi
	movl	16(%esp),%edx
	addl	%ebp,%edi
	# 20_39 20

	movl	%esi,%ebp
	xorl	24(%esp),%edx
	xorl	%eax,%ebp
	xorl	48(%esp),%edx
	xorl	%ebx,%ebp
	xorl	4(%esp),%edx
	roll	$1,%edx
	addl	%ebp,%ecx
	rorl	$2,%esi
	movl	%edi,%ebp
	roll	$5,%ebp
	movl	%edx,16(%esp)
	leal	1859775393(%edx,%ecx,1),%edx
	movl	20(%esp),%ecx
	addl	%ebp,%edx
	# 20_39 21

	movl	%edi,%ebp
	xorl	28(%esp),%ecx
	xorl	%esi,%ebp
	xorl	52(%esp),%ecx
	xorl	%eax,%ebp
	xorl	8(%esp),%ecx
	roll	$1,%ecx
	addl	%ebp,%ebx
	rorl	$2,%edi
	movl	%edx,%ebp
	roll	$5,%ebp
	movl	%ecx,20(%esp)
	leal	1859775393(%ecx,%ebx,1),%ecx
	movl	24(%esp),%ebx
	addl	%ebp,%ecx
	# 20_39 22

	movl	%edx,%ebp
	xorl	32(%esp),%ebx
	xorl	%edi,%ebp
	xorl	56(%esp),%ebx
	xorl	%esi,%ebp
	xorl	12(%esp),%ebx
	roll	$1,%ebx
	addl	%ebp,%eax
	rorl	$2,%edx
	movl	%ecx,%ebp
	roll	$5,%ebp
	movl	%ebx,24(%esp)
	leal	1859775393(%ebx,%eax,1),%ebx
	movl	28(%esp),%eax
	addl	%ebp,%ebx
	# 20_39 23

	movl	%ecx,%ebp
	xorl	36(%esp),%eax
	xorl	%edx,%ebp
	xorl	60(%esp),%eax
	xorl	%edi,%ebp
	xorl	16(%esp),%eax
	roll	$1,%eax
	addl	%ebp,%esi
	rorl	$2,%ecx
	movl	%ebx,%ebp
	roll	$5,%ebp
	movl	%eax,28(%esp)
	leal	1859775393(%eax,%esi,1),%eax
	movl	32(%esp),%esi
	addl	%ebp,%eax
	# 20_39 24

	movl	%ebx,%ebp
	xorl	40(%esp),%esi
	xorl	%ecx,%ebp
	xorl	(%esp),%esi
	xorl	%edx,%ebp
	xorl	20(%esp),%esi
	roll	$1,%esi
	addl	%ebp,%edi
	rorl	$2,%ebx
	movl	%eax,%ebp
	roll	$5,%ebp
	movl	%esi,32(%esp)
	leal	1859775393(%esi,%edi,1),%esi
	movl	36(%esp),%edi
	addl	%ebp,%esi
	# 20_39 25

	movl	%eax,%ebp
	xorl	44(%esp),%edi
	xorl	%ebx,%ebp
	xorl	4(%esp),%edi
	xorl	%ecx,%ebp
	xorl	24(%esp),%edi
	roll	$1,%edi
	addl	%ebp,%edx
	rorl	$2,%eax
	movl	%esi,%ebp
	roll	$5,%ebp
	movl	%edi,36(%esp)
	leal	1859775393(%edi,%edx,1),%edi
	movl	40(%esp),%edx
	addl	%ebp,%edi
	# 20_39 26

	movl	%esi,%ebp
	xorl	48(%esp),%edx
	xorl	%eax,%ebp
	xorl	8(%esp),%edx
	xorl	%ebx,%ebp
	xorl	28(%esp),%edx
	roll	$1,%edx
	addl	%ebp,%ecx
	rorl	$2,%esi
	movl	%edi,%ebp
	roll	$5,%ebp
	movl	%edx,40(%esp)
	leal	1859775393(%edx,%ecx,1),%edx
	movl	44(%esp),%ecx
	addl	%ebp,%edx
	# 20_39 27

	movl	%edi,%ebp
	xorl	52(%esp),%ecx
	xorl	%esi,%ebp
	xorl	12(%esp),%ecx
	xorl	%eax,%ebp
	xorl	32(%esp),%ecx
	roll	$1,%ecx
	addl	%ebp,%ebx
	rorl	$2,%edi
	movl	%edx,%ebp
	roll	$5,%ebp
	movl	%ecx,44(%esp)
	leal	1859775393(%ecx,%ebx,1),%ecx
	movl	48(%esp),%ebx
	addl	%ebp,%ecx
	# 20_39 28

	movl	%edx,%ebp
	xorl	56(%esp),%ebx
	xorl	%edi,%ebp
	xorl	16(%esp),%ebx
	xorl	%esi,%ebp
	xorl	36(%esp),%ebx
	roll	$1,%ebx
	addl	%ebp,%eax
	rorl	$2,%edx
	movl	%ecx,%ebp
	roll	$5,%ebp
	movl	%ebx,48(%esp)
	leal	1859775393(%ebx,%eax,1),%ebx
	movl	52(%esp),%eax
	addl	%ebp,%ebx
	# 20_39 29

	movl	%ecx,%ebp
	xorl	60(%esp),%eax
	xorl	%edx,%ebp
	xorl	20(%esp),%eax
	xorl	%edi,%ebp
	xorl	40(%esp),%eax
	roll	$1,%eax
	addl	%ebp,%esi
	rorl	$2,%ecx
	movl	%ebx,%ebp
	roll	$5,%ebp
	movl	%eax,52(%esp)
	leal	1859775393(%eax,%esi,1),%eax
	movl	56(%esp),%esi
	addl	%ebp,%eax
	# 20_39 30

	movl	%ebx,%ebp
	xorl	(%esp),%esi
	xorl	%ecx,%ebp
	xorl	24(%esp),%esi
	xorl	%edx,%ebp
	xorl	44(%esp),%esi
	roll	$1,%esi
	addl	%ebp,%edi
	rorl	$2,%ebx
	movl	%eax,%ebp
	roll	$5,%ebp
	movl	%esi,56(%esp)
	leal	1859775393(%esi,%edi,1),%esi
	movl	60(%esp),%edi
	addl	%ebp,%esi
	# 20_39 31

	movl	%eax,%ebp
	xorl	4(%esp),%edi
	xorl	%ebx,%ebp
	xorl	28(%esp),%edi
	xorl	%ecx,%ebp
	xorl	48(%esp),%edi
	roll	$1,%edi
	addl	%ebp,%edx
	rorl	$2,%eax
	movl	%esi,%ebp
	roll	$5,%ebp
	movl	%edi,60(%esp)
	leal	1859775393(%edi,%edx,1),%edi
	movl	(%esp),%edx
	addl	%ebp,%edi
	# 20_39 32

	movl	%esi,%ebp
	xorl	8(%esp),%edx
	xorl	%eax,%ebp
	xorl	32(%esp),%edx
	xorl	%ebx,%ebp
	xorl	52(%esp),%edx
	roll	$1,%edx
	addl	%ebp,%ecx
	rorl	$2,%esi
	movl	%edi,%ebp
	roll	$5,%ebp
	movl	%edx,(%esp)
	leal	1859775393(%edx,%ecx,1),%edx
	movl	4(%esp),%ecx
	addl	%ebp,%edx
	# 20_39 33

	movl	%edi,%ebp
	xorl	12(%esp),%ecx
	xorl	%esi,%ebp
	xorl	36(%esp),%ecx
	xorl	%eax,%ebp
	xorl	56(%esp),%ecx
	roll	$1,%ecx
	addl	%ebp,%ebx
	rorl	$2,%edi
	movl	%edx,%ebp
	roll	$5,%ebp
	movl	%ecx,4(%esp)
	leal	1859775393(%ecx,%ebx,1),%ecx
	movl	8(%esp),%ebx
	addl	%ebp,%ecx
	# 20_39 34

	movl	%edx,%ebp
	xorl	16(%esp),%ebx
	xorl	%edi,%ebp
	xorl	40(%esp),%ebx
	xorl	%esi,%ebp
	xorl	60(%esp),%ebx
	roll	$1,%ebx
	addl	%ebp,%eax
	rorl	$2,%edx
	movl	%ecx,%ebp
	roll	$5,%ebp
	movl	%ebx,8(%esp)
	leal	1859775393(%ebx,%eax,1),%ebx
	movl	12(%esp),%eax
	addl	%ebp,%ebx
	# 20_39 35

	movl	%ecx,%ebp
	xorl	20(%esp),%eax
	xorl	%edx,%ebp
	xorl	44(%esp),%eax
	xorl	%edi,%ebp
	xorl	(%esp),%eax
	roll	$1,%eax
	addl	%ebp,%esi
	rorl	$2,%ecx
	movl	%ebx,%ebp
	roll	$5,%ebp
	movl	%eax,12(%esp)
	leal	1859775393(%eax,%esi,1),%eax
	movl	16(%esp),%esi
	addl	%ebp,%eax
	# 20_39 36

	movl	%ebx,%ebp
	xorl	24(%esp),%esi
	xorl	%ecx,%ebp
	xorl	48(%esp),%esi
	xorl	%edx,%ebp
	xorl	4(%esp),%esi
	roll	$1,%esi
	addl	%ebp,%edi
	rorl	$2,%ebx
	movl	%eax,%ebp
	roll	$5,%ebp
	movl	%esi,16(%esp)
	leal	1859775393(%esi,%edi,1),%esi
	movl	20(%esp),%edi
	addl	%ebp,%esi
	# 20_39 37

	movl	%eax,%ebp
	xorl	28(%esp),%edi
	xorl	%ebx,%ebp
	xorl	52(%esp),%edi
	xorl	%ecx,%ebp
	xorl	8(%esp),%edi
	roll	$1,%edi
	addl	%ebp,%edx
	rorl	$2,%eax
	movl	%esi,%ebp
	roll	$5,%ebp
	movl	%edi,20(%esp)
	leal	1859775393(%edi,%edx,1),%edi
	movl	24(%esp),%edx
	addl	%ebp,%edi
	# 20_39 38

	movl	%esi,%ebp
	xorl	32(%esp),%edx
	xorl	%eax,%ebp
	xorl	56(%esp),%edx
	xorl	%ebx,%ebp
	xorl	12(%esp),%edx
	roll	$1,%edx
	addl	%ebp,%ecx
	rorl	$2,%esi
	movl	%edi,%ebp
	roll	$5,%ebp
	movl	%edx,24(%esp)
	leal	1859775393(%edx,%ecx,1),%edx
	movl	28(%esp),%ecx
	addl	%ebp,%edx
	# 20_39 39

	movl	%edi,%ebp
	xorl	36(%esp),%ecx
	xorl	%esi,%ebp
	xorl	60(%esp),%ecx
	xorl	%eax,%ebp
	xorl	16(%esp),%ecx
	roll	$1,%ecx
	addl	%ebp,%ebx
	rorl	$2,%edi
	movl	%edx,%ebp
	roll	$5,%ebp
	movl	%ecx,28(%esp)
	leal	1859775393(%ecx,%ebx,1),%ecx
	movl	32(%esp),%ebx
	addl	%ebp,%ecx
	# 40_59 40

	movl	%edi,%ebp
	xorl	40(%esp),%ebx
	xorl	%esi,%ebp
	xorl	(%esp),%ebx
	andl	%edx,%ebp
	xorl	20(%esp),%ebx
	roll	$1,%ebx
	addl	%eax,%ebp
	rorl	$2,%edx
	movl	%ecx,%eax
	roll	$5,%eax
	movl	%ebx,32(%esp)
	leal	2400959708(%ebx,%ebp,1),%ebx
	movl	%edi,%ebp
	addl	%eax,%ebx
	andl	%esi,%ebp
	movl	36(%esp),%eax
	addl	%ebp,%ebx
	# 40_59 41

	movl	%edx,%ebp
	xorl	44(%esp),%eax
	xorl	%edi,%ebp
	xorl	4(%esp),%eax
	andl	%ecx,%ebp
	xorl	24(%esp),%eax
	roll	$1,%eax
	addl	%esi,%ebp
	rorl	$2,%ecx
	movl	%ebx,%esi
	roll	$5,%esi
	movl	%eax,36(%esp)
	leal	2400959708(%eax,%ebp,1),%eax
	movl	%edx,%ebp
	addl	%esi,%eax
	andl	%edi,%ebp
	movl	40(%esp),%esi
	addl	%ebp,%eax
	# 40_59 42

	movl	%ecx,%ebp
	xorl	48(%esp),%esi
	xorl	%edx,%ebp
	xorl	8(%esp),%esi
	andl	%ebx,%ebp
	xorl	28(%esp),%esi
	roll	$1,%esi
	addl	%edi,%ebp
	rorl	$2,%ebx
	movl	%eax,%edi
	roll	$5,%edi
	movl	%esi,40(%esp)
	leal	2400959708(%esi,%ebp,1),%esi
	movl	%ecx,%ebp
	addl	%edi,%esi
	andl	%edx,%ebp
	movl	44(%esp),%edi
	addl	%ebp,%esi
	# 40_59 43

	movl	%ebx,%ebp
	xorl	52(%esp),%edi
	xorl	%ecx,%ebp
	xorl	12(%esp),%edi
	andl	%eax,%ebp
	xorl	32(%esp),%edi
	roll	$1,%edi
	addl	%edx,%ebp
	rorl	$2,%eax
	movl	%esi,%edx
	roll	$5,%edx
	movl	%edi,44(%esp)
	leal	2400959708(%edi,%ebp,1),%edi
	movl	%ebx,%ebp
	addl	%edx,%edi
	andl	%ecx,%ebp
	movl	48(%esp),%edx
	addl	%ebp,%edi
	# 40_59 44

	movl	%eax,%ebp
	xorl	56(%esp),%edx
	xorl	%ebx,%ebp
	xorl	16(%esp),%edx
	andl	%esi,%ebp
	xorl	36(%esp),%edx
	roll	$1,%edx
	addl	%ecx,%ebp
	rorl	$2,%esi
	movl	%edi,%ecx
	roll	$5,%ecx
	movl	%edx,48(%esp)
	leal	2400959708(%edx,%ebp,1),%edx
	movl	%eax,%ebp
	addl	%ecx,%edx
	andl	%ebx,%ebp
	movl	52(%esp),%ecx
	addl	%ebp,%edx
	# 40_59 45

	movl	%esi,%ebp
	xorl	60(%esp),%ecx
	xorl	%eax,%ebp
	xorl	20(%esp),%ecx
	andl	%edi,%ebp
	xorl	40(%esp),%ecx
	roll	$1,%ecx
	addl	%ebx,%ebp
	rorl	$2,%edi
	movl	%edx,%ebx
	roll	$5,%ebx
	movl	%ecx,52(%esp)
	leal	2400959708(%ecx,%ebp,1),%ecx
	movl	%esi,%ebp
	addl	%ebx,%ecx
	andl	%eax,%ebp
	movl	56(%esp),%ebx
	addl	%ebp,%ecx
	# 40_59 46

	movl	%edi,%ebp
	xorl	(%esp),%ebx
	xorl	%esi,%ebp
	xorl	24(%esp),%ebx
	andl	%edx,%ebp
	xorl	44(%esp),%ebx
	roll	$1,%ebx
	addl	%eax,%ebp
	rorl	$2,%edx
	movl	%ecx,%eax
	roll	$5,%eax
	movl	%ebx,56(%esp)
	leal	2400959708(%ebx,%ebp,1),%ebx
	movl	%edi,%ebp
	addl	%eax,%ebx
	andl	%esi,%ebp
	movl	60(%esp),%eax
	addl	%ebp,%ebx
	# 40_59 47

	movl	%edx,%ebp
	xorl	4(%esp),%eax
	xorl	%edi,%ebp
	xorl	28(%esp),%eax
	andl	%ecx,%ebp
	xorl	48(%esp),%eax
	roll	$1,%eax
	addl	%esi,%ebp
	rorl	$2,%ecx
	movl	%ebx,%esi
	roll	$5,%esi
	movl	%eax,60(%esp)
	leal	2400959708(%eax,%ebp,1),%eax
	movl	%edx,%ebp
	addl	%esi,%eax
	andl	%edi,%ebp
	movl	(%esp),%esi
	addl	%ebp,%eax
	# 40_59 48

	movl	%ecx,%ebp
	xorl	8(%esp),%esi
	xorl	%edx,%ebp
	xorl	32(%esp),%esi
	andl	%ebx,%ebp
	xorl	52(%esp),%esi
	roll	$1,%esi
	addl	%edi,%ebp
	rorl	$2,%ebx
	movl	%eax,%edi
	roll	$5,%edi
	movl	%esi,(%esp)
	leal	2400959708(%esi,%ebp,1),%esi
	movl	%ecx,%ebp
	addl	%edi,%esi
	andl	%edx,%ebp
	movl	4(%esp),%edi
	addl	%ebp,%esi
	# 40_59 49

	movl	%ebx,%ebp
	xorl	12(%esp),%edi
	xorl	%ecx,%ebp
	xorl	36(%esp),%edi
	andl	%eax,%ebp
	xorl	56(%esp),%edi
	roll	$1,%edi
	addl	%edx,%ebp
	rorl	$2,%eax
	movl	%esi,%edx
	roll	$5,%edx
	movl	%edi,4(%esp)
	leal	2400959708(%edi,%ebp,1),%edi
	movl	%ebx,%ebp
	addl	%edx,%edi
	andl	%ecx,%ebp
	movl	8(%esp),%edx
	addl	%ebp,%edi
	# 40_59 50

	movl	%eax,%ebp
	xorl	16(%esp),%edx
	xorl	%ebx,%ebp
	xorl	40(%esp),%edx
	andl	%esi,%ebp
	xorl	60(%esp),%edx
	roll	$1,%edx
	addl	%ecx,%ebp
	rorl	$2,%esi
	movl	%edi,%ecx
	roll	$5,%ecx
	movl	%edx,8(%esp)
	leal	2400959708(%edx,%ebp,1),%edx
	movl	%eax,%ebp
	addl	%ecx,%edx
	andl	%ebx,%ebp
	movl	12(%esp),%ecx
	addl	%ebp,%edx
	# 40_59 51

	movl	%esi,%ebp
	xorl	20(%esp),%ecx
	xorl	%eax,%ebp
	xorl	44(%esp),%ecx
	andl	%edi,%ebp
	xorl	(%esp),%ecx
	roll	$1,%ecx
	addl	%ebx,%ebp
	rorl	$2,%edi
	movl	%edx,%ebx
	roll	$5,%ebx
	movl	%ecx,12(%esp)
	leal	2400959708(%ecx,%ebp,1),%ecx
	movl	%esi,%ebp
	addl	%ebx,%ecx
	andl	%eax,%ebp
	movl	16(%esp),%ebx
	addl	%ebp,%ecx
	# 40_59 52

	movl	%edi,%ebp
	xorl	24(%esp),%ebx
	xorl	%esi,%ebp
	xorl	48(%esp),%ebx
	andl	%edx,%ebp
	xorl	4(%esp),%ebx
	roll	$1,%ebx
	addl	%eax,%ebp
	rorl	$2,%edx
	movl	%ecx,%eax
	roll	$5,%eax
	movl	%ebx,16(%esp)
	leal	2400959708(%ebx,%ebp,1),%ebx
	movl	%edi,%ebp
	addl	%eax,%ebx
	andl	%esi,%ebp
	movl	20(%esp),%eax
	addl	%ebp,%ebx
	# 40_59 53

	movl	%edx,%ebp
	xorl	28(%esp),%eax
	xorl	%edi,%ebp
	xorl	52(%esp),%eax
	andl	%ecx,%ebp
	xorl	8(%esp),%eax
	roll	$1,%eax
	addl	%esi,%ebp
	rorl	$2,%ecx
	movl	%ebx,%esi
	roll	$5,%esi
	movl	%eax,20(%esp)
	leal	2400959708(%eax,%ebp,1),%eax
	movl	%edx,%ebp
	addl	%esi,%eax
	andl	%edi,%ebp
	movl	24(%esp),%esi
	addl	%ebp,%eax
	# 40_59 54

	movl	%ecx,%ebp
	xorl	32(%esp),%esi
	xorl	%edx,%ebp
	xorl	56(%esp),%esi
	andl	%ebx,%ebp
	xorl	12(%esp),%esi
	roll	$1,%esi
	addl	%edi,%ebp
	rorl	$2,%ebx
	movl	%eax,%edi
	roll	$5,%edi
	movl	%esi,24(%esp)
	leal	2400959708(%esi,%ebp,1),%esi
	movl	%ecx,%ebp
	addl	%edi,%esi
	andl	%edx,%ebp
	movl	28(%esp),%edi
	addl	%ebp,%esi
	# 40_59 55

	movl	%ebx,%ebp
	xorl	36(%esp),%edi
	xorl	%ecx,%ebp
	xorl	60(%esp),%edi
	andl	%eax,%ebp
	xorl	16(%esp),%edi
	roll	$1,%edi
	addl	%edx,%ebp
	rorl	$2,%eax
	movl	%esi,%edx
	roll	$5,%edx
	movl	%edi,28(%esp)
	leal	2400959708(%edi,%ebp,1),%edi
	movl	%ebx,%ebp
	addl	%edx,%edi
	andl	%ecx,%ebp
	movl	32(%esp),%edx
	addl	%ebp,%edi
	# 40_59 56

	movl	%eax,%ebp
	xorl	40(%esp),%edx
	xorl	%ebx,%ebp
	xorl	(%esp),%edx
	andl	%esi,%ebp
	xorl	20(%esp),%edx
	roll	$1,%edx
	addl	%ecx,%ebp
	rorl	$2,%esi
	movl	%edi,%ecx
	roll	$5,%ecx
	movl	%edx,32(%esp)
	leal	2400959708(%edx,%ebp,1),%edx
	movl	%eax,%ebp
	addl	%ecx,%edx
	andl	%ebx,%ebp
	movl	36(%esp),%ecx
	addl	%ebp,%edx
	# 40_59 57

	movl	%esi,%ebp
	xorl	44(%esp),%ecx
	xorl	%eax,%ebp
	xorl	4(%esp),%ecx
	andl	%edi,%ebp
	xorl	24(%esp),%ecx
	roll	$1,%ecx
	addl	%ebx,%ebp
	rorl	$2,%edi
	movl	%edx,%ebx
	roll	$5,%ebx
	movl	%ecx,36(%esp)
	leal	2400959708(%ecx,%ebp,1),%ecx
	movl	%esi,%ebp
	addl	%ebx,%ecx
	andl	%eax,%ebp
	movl	40(%esp),%ebx
	addl	%ebp,%ecx
	# 40_59 58

	movl	%edi,%ebp
	xorl	48(%esp),%ebx
	xorl	%esi,%ebp
	xorl	8(%esp),%ebx
	andl	%edx,%ebp
	xorl	28(%esp),%ebx
	roll	$1,%ebx
	addl	%eax,%ebp
	rorl	$2,%edx
	movl	%ecx,%eax
	roll	$5,%eax
	movl	%ebx,40(%esp)
	leal	2400959708(%ebx,%ebp,1),%ebx
	movl	%edi,%ebp
	addl	%eax,%ebx
	andl	%esi,%ebp
	movl	44(%esp),%eax
	addl	%ebp,%ebx
	# 40_59 59

	movl	%edx,%ebp
	xorl	52(%esp),%eax
	xorl	%edi,%ebp
	xorl	12(%esp),%eax
	andl	%ecx,%ebp
	xorl	32(%esp),%eax
	roll	$1,%eax
	addl	%esi,%ebp
	rorl	$2,%ecx
	movl	%ebx,%esi
	roll	$5,%esi
	movl	%eax,44(%esp)
	leal	2400959708(%eax,%ebp,1),%eax
	movl	%edx,%ebp
	addl	%esi,%eax
	andl	%edi,%ebp
	movl	48(%esp),%esi
	addl	%ebp,%eax
	# 20_39 60

	movl	%ebx,%ebp
	xorl	56(%esp),%esi
	xorl	%ecx,%ebp
	xorl	16(%esp),%esi
	xorl	%edx,%ebp
	xorl	36(%esp),%esi
	roll	$1,%esi
	addl	%ebp,%edi
	rorl	$2,%ebx
	movl	%eax,%ebp
	roll	$5,%ebp
	movl	%esi,48(%esp)
	leal	3395469782(%esi,%edi,1),%esi
	movl	52(%esp),%edi
	addl	%ebp,%esi
	# 20_39 61

	movl	%eax,%ebp
	xorl	60(%esp),%edi
	xorl	%ebx,%ebp
	xorl	20(%esp),%edi
	xorl	%ecx,%ebp
	xorl	40(%esp),%edi
	roll	$1,%edi
	addl	%ebp,%edx
	rorl	$2,%eax
	movl	%esi,%ebp
	roll	$5,%ebp
	movl	%edi,52(%esp)
	leal	3395469782(%edi,%edx,1),%edi
	movl	56(%esp),%edx
	addl	%ebp,%edi
	# 20_39 62

	movl	%esi,%ebp
	xorl	(%esp),%edx
	xorl	%eax,%ebp
	xorl	24(%esp),%edx
	xorl	%ebx,%ebp
	xorl	44(%esp),%edx
	roll	$1,%edx
	addl	%ebp,%ecx
	rorl	$2,%esi
	movl	%edi,%ebp
	roll	$5,%ebp
	movl	%edx,56(%esp)
	leal	3395469782(%edx,%ecx,1),%edx
	movl	60(%esp),%ecx
	addl	%ebp,%edx
	# 20_39 63

	movl	%edi,%ebp
	xorl	4(%esp),%ecx
	xorl	%esi,%ebp
	xorl	28(%esp),%ecx
	xorl	%eax,%ebp
	xorl	48(%esp),%ecx
	roll	$1,%ecx
	addl	%ebp,%ebx
	rorl	$2,%edi
	movl	%edx,%ebp
	roll	$5,%ebp
	movl	%ecx,60(%esp)
	leal	3395469782(%ecx,%ebx,1),%ecx
	movl	(%esp),%ebx
	addl	%ebp,%ecx
	# 20_39 64

	movl	%edx,%ebp
	xorl	8(%esp),%ebx
	xorl	%edi,%ebp
	xorl	32(%esp),%ebx
	xorl	%esi,%ebp
	xorl	52(%esp),%ebx
	roll	$1,%ebx
	addl	%ebp,%eax
	rorl	$2,%edx
	movl	%ecx,%ebp
	roll	$5,%ebp
	movl	%ebx,(%esp)
	leal	3395469782(%ebx,%eax,1),%ebx
	movl	4(%esp),%eax
	addl	%ebp,%ebx
	# 20_39 65

	movl	%ecx,%ebp
	xorl	12(%esp),%eax
	xorl	%edx,%ebp
	xorl	36(%esp),%eax
	xorl	%edi,%ebp
	xorl	56(%esp),%eax
	roll	$1,%eax
	addl	%ebp,%esi
	rorl	$2,%ecx
	movl	%ebx,%ebp
	roll	$5,%ebp
	movl	%eax,4(%esp)
	leal	3395469782(%eax,%esi,1),%eax
	movl	8(%esp),%esi
	addl	%ebp,%eax
	# 20_39 66

	movl	%ebx,%ebp
	xorl	16(%esp),%esi
	xorl	%ecx,%ebp
	xorl	40(%esp),%esi
	xorl	%edx,%ebp
	xorl	60(%esp),%esi
	roll	$1,%esi
	addl	%ebp,%edi
	rorl	$2,%ebx
	movl	%eax,%ebp
	roll	$5,%ebp
	movl	%esi,8(%esp)
	leal	3395469782(%esi,%edi,1),%esi
	movl	12(%esp),%edi
	addl	%ebp,%esi
	# 20_39 67

	movl	%eax,%ebp
	xorl	20(%esp),%edi
	xorl	%ebx,%ebp
	xorl	44(%esp),%edi
	xorl	%ecx,%ebp
	xorl	(%esp),%edi
	roll	$1,%edi
	addl	%ebp,%edx
	rorl	$2,%eax
	movl	%esi,%ebp
	roll	$5,%ebp
	movl	%edi,12(%esp)
	leal	3395469782(%edi,%edx,1),%edi
	movl	16(%esp),%edx
	addl	%ebp,%edi
	# 20_39 68

	movl	%esi,%ebp
	xorl	24(%esp),%edx
	xorl	%eax,%ebp
	xorl	48(%esp),%edx
	xorl	%ebx,%ebp
	xorl	4(%esp),%edx
	roll	$1,%edx
	addl	%ebp,%ecx
	rorl	$2,%esi
	movl	%edi,%ebp
	roll	$5,%ebp
	movl	%edx,16(%esp)
	leal	3395469782(%edx,%ecx,1),%edx
	movl	20(%esp),%ecx
	addl	%ebp,%edx
	# 20_39 69

	movl	%edi,%ebp
	xorl	28(%esp),%ecx
	xorl	%esi,%ebp
	xorl	52(%esp),%ecx
	xorl	%eax,%ebp
	xorl	8(%esp),%ecx
	roll	$1,%ecx
	addl	%ebp,%ebx
	rorl	$2,%edi
	movl	%edx,%ebp
	roll	$5,%ebp
	movl	%ecx,20(%esp)
	leal	3395469782(%ecx,%ebx,1),%ecx
	movl	24(%esp),%ebx
	addl	%ebp,%ecx
	# 20_39 70

	movl	%edx,%ebp
	xorl	32(%esp),%ebx
	xorl	%edi,%ebp
	xorl	56(%esp),%ebx
	xorl	%esi,%ebp
	xorl	12(%esp),%ebx
	roll	$1,%ebx
	addl	%ebp,%eax
	rorl	$2,%edx
	movl	%ecx,%ebp
	roll	$5,%ebp
	movl	%ebx,24(%esp)
	leal	3395469782(%ebx,%eax,1),%ebx
	movl	28(%esp),%eax
	addl	%ebp,%ebx
	# 20_39 71

	movl	%ecx,%ebp
	xorl	36(%esp),%eax
	xorl	%edx,%ebp
	xorl	60(%esp),%eax
	xorl	%edi,%ebp
	xorl	16(%esp),%eax
	roll	$1,%eax
	addl	%ebp,%esi
	rorl	$2,%ecx
	movl	%ebx,%ebp
	roll	$5,%ebp
	movl	%eax,28(%esp)
	leal	3395469782(%eax,%esi,1),%eax
	movl	32(%esp),%esi
	addl	%ebp,%eax
	# 20_39 72

	movl	%ebx,%ebp
	xorl	40(%esp),%esi
	xorl	%ecx,%ebp
	xorl	(%esp),%esi
	xorl	%edx,%ebp
	xorl	20(%esp),%esi
	roll	$1,%esi
	addl	%ebp,%edi
	rorl	$2,%ebx
	movl	%eax,%ebp
	roll	$5,%ebp
	movl	%esi,32(%esp)
	leal	3395469782(%esi,%edi,1),%esi
	movl	36(%esp),%edi
	addl	%ebp,%esi
	# 20_39 73

	movl	%eax,%ebp
	xorl	44(%esp),%edi
	xorl	%ebx,%ebp
	xorl	4(%esp),%edi
	xorl	%ecx,%ebp
	xorl	24(%esp),%edi
	roll	$1,%edi
	addl	%ebp,%edx
	rorl	$2,%eax
	movl	%esi,%ebp
	roll	$5,%ebp
	movl	%edi,36(%esp)
	leal	3395469782(%edi,%edx,1),%edi
	movl	40(%esp),%edx
	addl	%ebp,%edi
	# 20_39 74

	movl	%esi,%ebp
	xorl	48(%esp),%edx
	xorl	%eax,%ebp
	xorl	8(%esp),%edx
	xorl	%ebx,%ebp
	xorl	28(%esp),%edx
	roll	$1,%edx
	addl	%ebp,%ecx
	rorl	$2,%esi
	movl	%edi,%ebp
	roll	$5,%ebp
	movl	%edx,40(%esp)
	leal	3395469782(%edx,%ecx,1),%edx
	movl	44(%esp),%ecx
	addl	%ebp,%edx
	# 20_39 75

	movl	%edi,%ebp
	xorl	52(%esp),%ecx
	xorl	%esi,%ebp
	xorl	12(%esp),%ecx
	xorl	%eax,%ebp
	xorl	32(%esp),%ecx
	roll	$1,%ecx
	addl	%ebp,%ebx
	rorl	$2,%edi
	movl	%edx,%ebp
	roll	$5,%ebp
	movl	%ecx,44(%esp)
	leal	3395469782(%ecx,%ebx,1),%ecx
	movl	48(%esp),%ebx
	addl	%ebp,%ecx
	# 20_39 76

	movl	%edx,%ebp
	xorl	56(%esp),%ebx
	xorl	%edi,%ebp
	xorl	16(%esp),%ebx
	xorl	%esi,%ebp
	xorl	36(%esp),%ebx
	roll	$1,%ebx
	addl	%ebp,%eax
	rorl	$2,%edx
	movl	%ecx,%ebp
	roll	$5,%ebp
	movl	%ebx,48(%esp)
	leal	3395469782(%ebx,%eax,1),%ebx
	movl	52(%esp),%eax
	addl	%ebp,%ebx
	# 20_39 77

	movl	%ecx,%ebp
	xorl	60(%esp),%eax
	xorl	%edx,%ebp
	xorl	20(%esp),%eax
	xorl	%edi,%ebp
	xorl	40(%esp),%eax
	roll	$1,%eax
	addl	%ebp,%esi
	rorl	$2,%ecx
	movl	%ebx,%ebp
	roll	$5,%ebp
	leal	3395469782(%eax,%esi,1),%eax
	movl	56(%esp),%esi
	addl	%ebp,%eax
	# 20_39 78

	movl	%ebx,%ebp
	xorl	(%esp),%esi
	xorl	%ecx,%ebp
	xorl	24(%esp),%esi
	xorl	%edx,%ebp
	xorl	44(%esp),%esi
	roll	$1,%esi
	addl	%ebp,%edi
	rorl	$2,%ebx
	movl	%eax,%ebp
	roll	$5,%ebp
	leal	3395469782(%esi,%edi,1),%esi
	movl	60(%esp),%edi
	addl	%ebp,%esi
	# 20_39 79

	movl	%eax,%ebp
	xorl	4(%esp),%edi
	xorl	%ebx,%ebp
	xorl	28(%esp),%edi
	xorl	%ecx,%ebp
	xorl	48(%esp),%edi
	roll	$1,%edi
	addl	%ebp,%edx
	rorl	$2,%eax
	movl	%esi,%ebp
	roll	$5,%ebp
	leal	3395469782(%edi,%edx,1),%edi
	addl	%ebp,%edi
	movl	96(%esp),%ebp
	movl	100(%esp),%edx
	addl	(%ebp),%edi
	addl	4(%ebp),%esi
	addl	8(%ebp),%eax
	addl	12(%ebp),%ebx
	addl	16(%ebp),%ecx
	movl	%edi,(%ebp)
	addl	$64,%edx
	movl	%esi,4(%ebp)
	cmpl	104(%esp),%edx
	movl	%eax,8(%ebp)
	movl	%ecx,%edi
	movl	%ebx,12(%ebp)
	movl	%edx,%esi
	movl	%ecx,16(%ebp)
	jb	L000loop
	addl	$76,%esp
	popl	%edi
	popl	%esi
	popl	%ebx
	popl	%ebp
	ret
.byte	83,72,65,49,32,98,108,111,99,107,32,116,114,97,110,115
.byte	102,111,114,109,32,102,111,114,32,120,56,54,44,32,67,82
.byte	89,80,84,79,71,65,77,83,32,98,121,32,60,97,112,112
.byte	114,111,64,111,112,101,110,115,115,108,46,111,114,103,62,0
