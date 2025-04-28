/*
 * memlib.c - a module that simulates the memory system.  Needed because it 
 *            allows us to interleave calls from the student's malloc package 
 *            with the system's malloc package in libc.
 * 
 * 왜 이러한 구조를 쓰는지?
 * 실제 시스템 메모리(sbrk,mmap)를 직접 건드리면 위험하고 복잡하니
 * 학생들이 연습할 수 있게 가짜 메모리 모델을 제공
 */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <sys/mman.h>
#include <string.h>
#include <errno.h>

#include "memlib.h"
#include "config.h"

/* private variables */
static char *mem_start_brk;  /* points to first byte of heap */
static char *mem_brk;        /* points to last byte of heap */
static char *mem_max_addr;   /* largest legal heap address */ 

/* 
 * mem_init - initialize the memory system model

malloc(MAX_HEAP)로 힙 전체를 메모리 한 번에 할당한다.

mem_start_brk = 힙의 시작 주소
mem_brk = 현재 힙의 끝 (처음엔 start와 같음)
mem_max_addr = 힙의 최대 허용 주소
 */
void mem_init(void)
{
    /* allocate the storage we will use to model the available VM */
    if ((mem_start_brk = (char *)malloc(MAX_HEAP)) == NULL) {
	fprintf(stderr, "mem_init_vm: malloc error\n");
	exit(1);
    //만약 메모리 확보에 실패했을 때는 에러 메세지 출력 후 종료
    }

    mem_max_addr = mem_start_brk + MAX_HEAP;  /* max legal heap address */
    /*
    MAX_HEAP은 초기 설정해준 heap의 최대크기
    시작 주소에서 최대 크기 더해주면 
    
    =>"힙의 최대 범위 끝 주소"
    */
    mem_brk = mem_start_brk;     
    /*
    mem_start_brk = "힙 영역의 시작 주소"
    
    현재 힙의 끝을 가리키는 mem_brk 포인터를 
    힙 시작 위치로 초기화 한다.

    즉 , "현재는 힙이 비어있는 상태" */
}

/* 
 * mem_deinit - free the storage used by the memory system model
 */
void mem_deinit(void)
{
    free(mem_start_brk);
    /*
    할당 해제? malloc으로 확보했던 가상 힙 공간 해제

    만약 안전하게 작성하고 싶으면 
    보통 mem_start_brk = NULL;

    */  

}

/*
 * mem_reset_brk - reset the simulated brk pointer to make an empty heap
 */
void mem_reset_brk()
{
    mem_brk = mem_start_brk;
    /*
    
    현재 사용 중이던 힙 영역을 "아무것도 없는 상태" 리셋
    
    끝 주소  = 시작주소
    
    리셋 해주기


    mem_start_brk = "힙의 태초의 위치"
    mem_brk = "힙의 현재 사용 경계선"
    */

}

/* 
 * mem_sbrk - simple model of the sbrk function. Extends the heap 
 *    by incr bytes and returns the start address of the new area. In
 *    this model, the heap cannot be shrunk.
 */
void *mem_sbrk(int incr) 
{

    /*
    
    


    */
    char *old_brk = mem_brk;
    /*힙의 현재 끝 주소를 복사 해두기

    mem_sbrk()는 "현재 힙 끝" 기준으로 incr 만큼 확장
    사용자는 "확장하기 전 위치를" 알아야 한다
    */

    if ( (incr < 0) || ((mem_brk + incr) > mem_max_addr)) {

    /*확장 범위가 0보다 작거나 

    확장 후의 범위가 초기에 할당한 
    힙의 최대 범위 끝 주소를 초과하면 

    */
	errno = ENOMEM;
    //메모리 부족 에러코드
	fprintf(stderr, "ERROR: mem_sbrk failed. Ran out of memory...\n");
	return (void *)-1; //실패를 알리는 리턴
    }

    //에러가 없을 때의 처리
    mem_brk += incr;
    //현재 힙 끝 포인터를 incr 바이트만큼 증가 시키기
    return (void *)old_brk;

    /*
    
    처음에 저장했던 "확장 전" 힙 끝 주소
    새로 할당된 메모리 블록의 시작주소가 된다.
    */
}

/*
 * mem_heap_lo - return address of the first heap byte
 */
void *mem_heap_lo()
{
    /*
    
    가짜 힙 메모리의 가장 low 한 주소를 반환
    
    [왜 필요함?]
    
    *가끔 "힙이 어디서 시작하는지" 알고 싶을 때 사용
    *힙 전체를 검사하거나, 처음부터 순회할 때
    
    힙의 제일 앞 주소 주세용(mem_start_brk)!!!

    */
    return (void *)mem_start_brk;
}

/* 
 * mem_heap_hi - return address of last heap byte
 */
void *mem_heap_hi()
{   

/*

가짜 힙 메모리의 가장 마지막에 사용된 바이트의 주소 반환

[주의할 점]

mem_brk는 "힙 끝 바로 다음 주소" (다음 시작 주소)

실제로 마지막 사용된 바이트는 mem_brk-1
힙에서 실제로 마지막으로 쓴 메로리 주소 주세요!

*/
    return (void *)(mem_brk - 1);
}

/*
 * mem_heapsize() - returns the heap size in bytes
 */
size_t mem_heapsize() 
{
    return (size_t)(mem_brk - mem_start_brk);
    /*
    
    현재까지 힙에서 사용 중인 크기(바이트 단위) 반환

    [계산 방법]

    힙 시작 주소 -현재 힙 끝 주소
    => 얼마나 많은 메모리를 썼는지(몇 바이트나 힙을 사용?)
    
    */
}

/*
 * mem_pagesize() - returns the page size of the system
 */
size_t mem_pagesize()
{
    return (size_t)getpagesize();
    // 이 함수는 시스템 콜 !!!!


    /*
    시스템의 페이지 크기를 반환(처음에 할당?)

    (운영체제에서 관리하는 메모리 단위 크기)

    [왜 필요한가]
    *실제 시스템 메모리 관리는 "페이지 단위"
    페이지 사이즈를 알아야 더 최적화된 메모리 할당 전략
    
    *예를 들어 큰 블록을 할당할 때 
     페이지 사이즈를 고려해서 정렬

    
    */
}
