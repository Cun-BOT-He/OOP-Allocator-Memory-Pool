#pragma once
#ifndef _HBCALLOC_H
#define _HBCALLOC_H
#include <cstdlib>
#include <cstddef>

class MemoryPool_Alloc {
	
private:

	enum { __ALIGN = 8 };//С��������ϵ��߽�
	enum { __MAX_BYTES = 256 };//С��������Ͻ磬�������ڴ泬��256�ֽڣ���ֱ�ӵ���malloc,free��realloc
	enum { __NFREELISTS = __MAX_BYTES / __ALIGN };//freelists����
	union  obj  //freelists�Ľڵ�
	{
		union obj* Free_List_Link;
		char client_data[1];
	};
	//freelists
	static obj* Free_List[__NFREELISTS];
	//memrypool
	static char *start_free;
	static char *end_free;
	static size_t heap_size;
    //���ú���
    //��bytes�ϵ���8�ı���
	static size_t ROUND_UP(size_t bytes) {  

		if (bytes % 8 != 0)

		{

			bytes = bytes + (8 - (bytes % 8));

		}

		return bytes;

	}

	//���������С������ʹ�õ�n��free_list n��1����
	static size_t FREELIST_INDEX(size_t bytes) {

		return (((bytes)+__ALIGN - 1) / __ALIGN - 1);

	}

	//����һ����СΪn�Ķ��󣬲�����ʹ�ô�СΪn���������free_list
	static void *refill(size_t n);
	//����һ���ռ䣬������ʹ�ô�СΪn���������free_list
	//nobjs���ܻή�ͣ�������������ʱ
	static char *chunk_alloc(size_t size, size_t& nobjs);

public:
	static void *allocate(size_t bytes);
	static void deallocate(void *p, size_t bytes);
	static void *reallocate(void *p, size_t originalSize, size_t newSize);
	MemoryPool_Alloc() {};
	~MemoryPool_Alloc() {};
};
    //�������ֵ�趨

    char* MemoryPool_Alloc::start_free = 0;
    char* MemoryPool_Alloc::end_free = 0;
    size_t MemoryPool_Alloc::heap_size = 0;
	
    MemoryPool_Alloc::obj *MemoryPool_Alloc::Free_List[MemoryPool_Alloc::__NFREELISTS] = 
	{ 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 };

	void *MemoryPool_Alloc::allocate(size_t bytes) {
		
		//�������ڴ����256�ֽڣ���Ϊ�������ڴ��ϴ�ֱ�ӵ���malloc���������ڴ�

		if (bytes > __MAX_BYTES){
			void *p;
			p = malloc(bytes);
			return p;
		}
		else{
			size_t index;

			//Ѱ��free_list�к��ʵ�һ������ռ�ȥȡ�ڴ�
			index = FREELIST_INDEX(bytes);
			obj *my_free_list = Free_List[index];

			//�����Ӧ��free_listû���ڴ棬����ڴ����ȥȡ
			if (my_free_list == NULL){
				void *r = refill(ROUND_UP(bytes));
		        
				return r;
			}
			Free_List[index] = my_free_list->Free_List_Link;//���ڴ��ʹ��Ȩ����my_free_list

			return my_free_list;
		}
	}
	void MemoryPool_Alloc::deallocate(void *p, size_t bytes) {

		//��������ͷŵ��ڴ����256�ֽڣ���ֱ�ӵ���free

		if (bytes > __MAX_BYTES){
			free(p);
		}

		//���С��256�ֽڣ��������free_list��

		obj* my_free_list;
		size_t index;

		index = FREELIST_INDEX(bytes);
		my_free_list = Free_List[index];
		obj *mynode = static_cast<obj*>(p);
		mynode->Free_List_Link = my_free_list;
		my_free_list = mynode;
	}
	void *MemoryPool_Alloc::reallocate(void *p, size_t originalSize, size_t newSize) {
		if (originalSize > __MAX_BYTES) {//ԭ�ڴ��С����256Bytes����ֱ��ʹ��realloc���������ϵͳheap����ռ�
			void *new_p = realloc(p, newSize);

			return new_p;
		}
		else {
			deallocate(p, originalSize);

			return allocate(newSize);
		}
	}
	void *MemoryPool_Alloc::refill(size_t n) {
		int nobjs = 20; //ÿ�����ӵĽڵ��������Ϊ�̶�Ϊ20��

		//���ڴ����ȡ��nobjs��n bytes����������
		char* chunk = chunk_alloc(n, nobjs);

		obj * volatile*my_lists;//ָ��free_list��ָ��
		obj *result;
		obj *current_obj, *next_obj;

		int i;
		//���ֻ���1�����飬���������ø��û�ʹ�ã�free_listû���½ڵ�
		if (1 == nobjs) return(chunk);
		//ѡ����ʵ�free_list�������½ڵ�
		my_lists = Free_List + FREELIST_INDEX(n);
		//��һ���ڴ�ֱ�ӷ��ظ��Ͷ�ʹ��
		result = (obj*)chunk;
		//ʣ�µ��ڴ����β�����������뵽��Ӧ��free_list��ȥ
		*my_lists = next_obj = (obj*)(chunk + n);
		for (i = 1; i<nobjs; i++){
			current_obj = next_obj;
			next_obj = (obj*)((char*)next_obj + n);
			if (nobjs - 1 == i){
				current_obj->Free_List_Link = NULL;
			}
			else{
				current_obj->Free_List_Link = next_obj;
			}
		}

		return result;
    }
	char *MemoryPool_Alloc::chunk_alloc(size_t size, size_t& nobjs) {

		char* result;
		size_t totalsize = size * nobjs;
		size_t bytesleft = end_free - start_free;

		if (bytesleft >= totalsize) {
			//����ڴ��ʣ�µĿռ��������Ҫ���ڴ�ռ�
			result = start_free;
			start_free += totalsize;

			return result;
		}
		else if (bytesleft >= size) {
			//����ڴ��ʣ�µĿռ����ٿ��Թ�һ��(������)��size
			//��ô���ڴ��ʣ�����ɷ��������free-list������nobjsΪ���������x(x < 20)
			nobjs = bytesleft / size;
			totalsize = nobjs * size;
			result = start_free;
			start_free += totalsize;

			return result;
		}

		else {    //�ڴ��ʣ��ռ���һ���ڴ涼�޷��ṩ
			//���ڴ���ڵ�ʣ���ڴ��ȷ�������ʵ�free_list
			if (bytesleft > 0) {
				obj **my_free_list;
				my_free_list = Free_List + FREELIST_INDEX(bytesleft);
				((obj*)start_free)->Free_List_Link = *my_free_list;
				*my_free_list = ((obj*)start_free);
			}
			//��ϵͳheap�����ڴ�
			size_t bytesToGet = 2 * totalsize + ROUND_UP(heap_size >> 4);
			start_free = (char*)malloc(bytesToGet);
			if (NULL == start_free) {
				//heap�ռ䲻�㣬mallocʧ��
				size_t i;

				obj** my_free_list;
				obj* temp;
				//������Ѱһ��freelist��"����δ�õ��㹻����ڴ��"
				for (i = size; i < __MAX_BYTES; i += __ALIGN) {
					my_free_list = Free_List + FREELIST_INDEX(i);
					temp = *my_free_list;
					if (NULL != temp) {
						*my_free_list = temp->Free_List_Link;
						start_free = (char*)temp;
						end_free = start_free + i;
						//�ݹ���ã�����nobjs
						return chunk_alloc(size, nobjs);

					}
				}
				//��ȫû�ڴ��ˣ�������
				end_free = NULL;
			}
			heap_size += bytesToGet;
			end_free = start_free + bytesToGet;
			
			//�ݹ���ã�����nobjs
			return chunk_alloc(size, nobjs);
		}
	}


#endif