/*
 * mm-naive.c - The fastest, least memory-efficient malloc package.

 * 내가 실제로 구현해야 하는 코드가 모여있는 파일
 * 
 * In this naive approach, a block is allocated by simply incrementing
 * the brk pointer.  A block is pure payload. There are no headers or
 * footers.  Blocks are never coalesced or reused. Realloc is
 * implemented directly using mm_malloc and mm_free.
 *
 * NOTE TO STUDENTS: Replace this header comment with your own header
 * comment that gives a high level description of your solution.
 */
#define WSIZE 4
#define DSIZE 8
#define CHUNKSIZE (1<<12)

#define MAX(x,y) ((x) > (y)? (x) :(y))
//x가 더 크면 x, y가 더 크면 y

#define PACK(size,alloc) ((size) | (alloc))
//이 pack(size, alloc)이라는 매크로는 size와 alloc의 or 연산을 수행 한다.
// 두 비트 중 하나라도 1이면 결과가 1이 되는 연산

/*
주소 p의 워드를 읽고 쓰기
unsigned int * 형 포인터로 타입 변환 후  *로 메모리 공간에 접근 하기
즉, 주소 읽기
*/
#define GET(p) (*(unsigned int *) (p))
#define PUT(p,val) (*(unsigned int *) (p) = (val)) //주소 변경 해주기
/*

[워드를 쓴다, PUT] 
-블록을 새로 만들 때 헤더/푸터에 (크기+할당상태)를 기록

[워드를 읽는다, GET]
-malloc/free 할 때 블록의 크기나 상태를 읽어 온다.

-realloc 할 때 ,블록 크기를 비교할 때도 사용
-현재 블록 크기 읽어와서, 새 요청 크기보다 작으면 복사

*/

//size 와 allocated 필드를 주소 p로 부터 읽어 온다.
#define GET_SIZE(p) (GET(p) & ~0x7)
/*
0x7은 이진수로 0000 0111 (맨 아랫자리 3비트가 1)
~0x7 은 1111 1000
*/
#define GET_ALLOC(p) (GET(p) & 0X1)

/*맨 마지막 비트를 본다. 
0000 0001 과 and 연산을 해서 할당되었는지 비트를 추출함
*/  


//bp는 사용자 데이터의 시작 주소(페이로드의 시작주소)
#define HDRP(bp)  ((char *)(bp) - WSIZE)
//bp에서 4바이트 만큼 뒤로 가면 헤더가 있다.

#define FTRP(bp)  ((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE)
/*bp에서 그 크기만큼 이동한 뒤, 그 위치에서 8바이트만큼 
다시 뒤로 가면 푸터가 있다.

왜나면 헤더에서 블록 사이즈 만큼 뒤로가면 그 블록의 헤더
푸터의 시작주소로 이동하려면 헤더블록 + 푸터블록 크기만큼 앞으로 가야됨

코어타임때 준석오빠가 물어본 부분임 !
*/


#define NEXT_BLKP(bp) ((char *)(bp) + GET_SIZE(((char *)(bp)-WSIZE))) 
#define PREV_BLKP(bp) ((char *)(bp) - GET_SIZE(((char *)(bp)-DSIZE)))
//DSZIE만큼 앞으로 가면 이전 블록의 footer알 수 있음
//푸터에 저장된 이전블록의 크기 알아보기

#include <stdio.h>
#include <stdlib.h> //size_t 쓸 수 있음
#include <assert.h>
#include <unistd.h>
#include <string.h>

#include "mm.h"
#include "memlib.h"

/*********************************************************
 * NOTE TO STUDENTS: Before you do anything else, please
 * provide your team information in the following struct.
 ********************************************************/
team_t team = {
    /* Team name */
    "team3",
    /* First member's full name */
    "molcham",
    /* First member's email address */
    "sonchaemin89@gmail.com",
    /* Second member's full name (leave blank if none) */
    "",
    /* Second member's email address (leave blank if none) */
    ""
};

/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~0x7)


#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))

/* 
 * mm_init - initialize the malloc package.
 */

static void *coalesce(void *bp)
{
  size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp)));
  size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
  size_t size = GET_SIZE(HDRP(bp));

  if(prev_alloc && next_alloc){

    return bp;

  }
  else if(prev_alloc && !next_alloc){
    size += GET_SIZE(HDRP(NEXT_BLKP((bp))));
    PUT(HDRP(bp),PACK(size,0));
    PUT(FTRP(bp),PACK(size,0));
  }
  else if(!prev_alloc && next_alloc){
    size += GET_SIZE(HDRP(PREV_BLKP(bp)));
    PUT(FTRP(bp),PACK(size,0));
    PUT(HDRP(PREV_BLKP(bp)), PACK(size,0));
    bp = PREV_BLKP(bp);
  }
  else{
    
    size += GET_SIZE(HDRP(PREV_BLKP(bp))) + GET_SIZE(FTRP(NEXT_BLKP(bp)));
    PUT(HDRP(PREV_BLKP(bp)), PACK(size,0));
    PUT(FTRP(NEXT_BLKP(bp)), PACK(size,0));
    bp = PREV_BLKP(bp);
    
  }

  return bp;
}

static void *extend_heap(size_t words)
//size_t는 unsigned integer 타입 stddef.h(표준 라이브러리 헤더) 안에 공식적 정의
{

    char *bp;
    size_t size;

    //정렬을 유지하기 위해 짝수 워드 할당 해주기
    size = (words % 2) ? (words+1) * WSIZE : words * WSIZE;
    if ((long)(bp = mem_sbrk(size)) == -1) // 확장 불가능 할 때,메모리 부족
       return NULL;


    //결국은 extend heap이니까 새로운 블록 생성해줘야 하는겨
    PUT(HDRP(bp),PACK(size,0));
    //bp위치에 새로운 블록을 만든다. , payload 시작 주소
    PUT(FTRP(bp),PACK(size,0));
    //같은 크기를 footer위치에도 기록해주기
    PUT(HDRP(NEXT_BLKP(bp)), PACK(0,1));
    //방금 만든 free block 뒤쪽에 새로운 에필로그 블록 크기 0, 할당 1

    //이전 블록이 free라면 병합해주기
    return coalesce(bp);



}
void *heap_listp;

int mm_init(void)
{
    //책에 있음
    if((heap_listp = mem_sbrk(4*WSIZE)) == (void *)-1) 
    //메모리 힙 영역 확장 함수
       return -1;
    PUT(heap_listp, 0);
    //매크로 정의 해준 PUT(p,val), 주소 변경
    PUT( (heap_listp + (1*WSIZE)) , PACK(DSIZE, 1));
    PUT( (heap_listp+ (2*WSIZE)) , PACK(DSIZE,1));
    PUT( (heap_listp + (3*WSIZE)), PACK(0,1));
    heap_listp+=(2*WSIZE);  
    //8바이트 만큼 증가시켜 주기?
    
    /*
    프로로그 블록 : 초기화용 더미 블록(payload가 없다)
    블록 합치기 로직을 단순하게 만들기 위해 존재하는 가짜 블록
    
    
    정확성 검사를 해주기 위해 초기화
    공간 효율성 검사할 때 초기화
    속도 측정 전에 초기화 전에  

    다음 테스트로 넘어갈 때 초기화 해주려고 쓰인다. 
    힙 내부를 갈아엎어서 초기화 해줄 때 쓰인다.
    테스트 하기 전에 힙 깔끔하게
    */

    if (extend_heap(CHUNKSIZE/WSIZE) == NULL)
       return -1;
    return 0;
}



static void *find_fit(size_t asize){
  //first-fit으로 찾기
  //맨 처음으로 fit한 블록 할당 해주기
  void *bp;
  for (bp =heap_listp; GET_SIZE(HDRP(bp))>0 ; bp = NEXT_BLKP(bp)) {
    if (!GET_ALLOC(HDRP(bp)) && (asize <= GET_SIZE(HDRP(bp)))) {
      //GET_ALLOC은 할당 비트 확인하기 
      //현재 블록 코드가 요청 크기보다 크거나 같아야 한다.
      return bp; 
    }
  }
  return NULL;
}

static void place(void *bp, size_t asize){
  size_t csize = GET_SIZE(HDRP(bp));
  //할당할 블록 전체 크기를 읽어온다.

  if((csize - asize) >= (2*DSIZE)){
    PUT(HDRP(bp), PACK(asize,1)); //헤더 주소 변경해주기
    PUT(FTRP(bp), PACK(asize,1)); //푸터 주소 변경해주기 

    bp =NEXT_BLKP(bp);
    PUT(HDRP(bp), PACK(csize-asize,0));
    PUT(FTRP(bp), PACK(csize-asize,0));
    //새 free블록은 할당 안 된 상태로 기록해야 한다. 
  }
  else{
    PUT(HDRP(bp),PACK(csize,1));
    PUT(FTRP(bp),PACK(csize,1));
    //PACK은 블록의 "크기"와 "할당 여부"를 한 번에 합쳐서 저장
    //남는 공간이 애매하면 그냥 전체 블록을 다 할당

  }
}

/* 
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size)
{
    size_t asize;
    //실제 할당할 조정된 크기
    size_t extendsize;
    //만약 free block 없으면 heap 확장할 크기
    char *bp;
    //할당할 블록의 payload 시작 주소 

    if(size == 0)
      return NULL;
    //mallo(0) 호출했을 때

    //오버헤드와 정렬조건을 맞추기 위해 블록 사이즈 조정
    if(size <= DSIZE)
       asize =2*DSIZE; 
       //최소 블록 크기 16 바이트 할당 해주기
    else
    //size가 DSIZE 초과면
       asize = DSIZE * ((size +(DSIZE) + (DSIZE-1))/ DSIZE);

    if ((bp = find_fit(asize)) != NULL){
        place(bp,asize);
        return bp;
        //나중에 이부분에서 next_fit , best_fit으로 고쳐보기
    }

    extendsize = MAX(asize,CHUNKSIZE);
    //만약 find-fit 반환값이 NULL이면 
    if((bp =extend_heap(extendsize/WSIZE)) == NULL)
    //메모리 확장이 불가능 할 때 호출해주기
      return NULL;
    place(bp,asize); // 확장에 성공하면 place로 새블록 할당 
    return bp;
}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *bp)
{
    //책에 있음
    size_t size = GET_SIZE(HDRP(bp));

    PUT(HDRP(bp),PACK(size,0));
    PUT(FTRP(bp),PACK(size,0));
    coalesce(bp);
}


/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
void *mm_realloc(void *bp, size_t size)
{
    //bp는 payload의 시작 주소
    //이걸 수정해줘야 성능이 올라감
    void *oldptr = bp;
    void *newptr;
    size_t copySize;

    if(bp == NULL){
      return mm_malloc(size);
    }

    if(size == 0){
      mm_free(bp);
      return NULL;
    }
    
    newptr = mm_malloc(size);
    if (newptr == NULL)
      return NULL;
    copySize = GET_SIZE(HDRP(oldptr)) -DSIZE;
    //블록 크기 +할당 여부 섞여 있는 값
    if (size < copySize)
      copySize = size;
    memcpy(newptr, oldptr, copySize);
    mm_free(oldptr);
    return newptr;
}














