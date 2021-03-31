#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include "dplist.h"

/*
 * definition of error codes
 * */
#define DPLIST_NO_ERROR 0
#define DPLIST_MEMORY_ERROR 1 // error due to mem alloc failure
#define DPLIST_INVALID_ERROR 2 //error due to a list operation applied on a NULL list 

#ifdef DEBUG
	#define DEBUG_PRINTF(...) 									         \
		do {											         \
			fprintf(stderr,"\nIn %s - function %s at line %d: ", __FILE__, __func__, __LINE__);	 \
			fprintf(stderr,__VA_ARGS__);								 \
			fflush(stderr);                                                                          \
                } while(0)
#else
	#define DEBUG_PRINTF(...) (void)0
#endif


#define DPLIST_ERR_HANDLER(condition,err_code)\
	do {						            \
            if ((condition)) DEBUG_PRINTF(#condition " failed\n");    \
            assert(!(condition));                                    \
        } while(0)

        
/*
 * The real definition of struct list / struct node
 */

struct dplist_node {
  dplist_node_t * prev, * next;
  void * element;
};


//struct dplist {
//  dplist_node_t * head;
//  void * (*element_copy)(void * src_element);
//  void (*element_free)(void ** element);
//  int (*element_compare)(void * x, void * y);
//};

dplist_t * dpl_create (// callback functions
			  void * (*element_copy)(void * src_element),
			  void (*element_free)(void ** element),
			  int (*element_compare)(void * x, void * y)
			  )
{
  dplist_t * list;
  list = malloc(sizeof(struct dplist));
  DPLIST_ERR_HANDLER(list==NULL,DPLIST_MEMORY_ERROR);
  list->head = NULL;  
  list->element_copy = element_copy;
  list->element_free = element_free;
  list->element_compare = element_compare; 
  return list;
}

void dpl_free(dplist_t ** list, bool free_element)
{
    // add your code here
    assert(*list!=NULL);
    dplist_node_t * dummy;
    if((**list).head==NULL){
        free((**list).head);
        (**list).head=NULL;
        free(*list);
        *list=NULL;}//empty list
    else{
          for(dummy = (**list).head; dummy->next != NULL ; )
          {
            dplist_node_t * delete_node = dummy;
            dummy = dummy->next;
            if(free_element){(**list).element_free(&(delete_node->element));};
            free(delete_node);
            delete_node=NULL;
          }
          if(free_element){(**list).element_free(&(dummy->element));};
          free(dummy);
          dummy=NULL;//free last element
          free(*list);
          *list=NULL;
        };
    assert(*list==NULL);
//    free(list);
}

dplist_t * dpl_insert_at_index(dplist_t * list, void * element, int index, bool insert_copy)
{
//    DPLIST_ERR_HANDLER(element==NULL,DPLIST_MEMORY_ERROR);
//if(element == NULL)
//    return list;
    // create a new element
    void * new_element;
    if(insert_copy)
    {
        new_element = list->element_copy(element);
    }
    else
    {
        new_element = element;
    }

    //create a new list_node
    dplist_node_t * ref_at_index, * list_node;
    DPLIST_ERR_HANDLER(list==NULL,DPLIST_INVALID_ERROR);
    list_node = malloc(sizeof(dplist_node_t));
    DPLIST_ERR_HANDLER(list_node==NULL,DPLIST_MEMORY_ERROR);
    list_node->element = new_element;

    if (list->head == NULL)
    { // if the list is empty, add a new element to be head
        list_node->prev = NULL;
        list_node->next = NULL;
        list->head = list_node;
    } else if (index <= 0)
    { // if the list is not empty, put the element in the front of that list
        list_node->prev = NULL;
        list_node->next = list->head;
        list->head->prev = list_node;
        list->head = list_node;
    } else
    {//if the list is not empty, put the element at the middle or end of that list
        ref_at_index = dpl_get_reference_at_index(list, index);
        assert( ref_at_index != NULL);
        if ((index+1) < dpl_size(list))
        { // if the list is not empty, put the element at the middle of that list
            list_node->prev = ref_at_index->prev;
            list_node->next = ref_at_index;
            ref_at_index->prev->next = list_node;
            ref_at_index->prev = list_node;

        } else
        { // if the list is not empty, put the element at the end of that list
            assert(ref_at_index->next == NULL);
            list_node->next = NULL;
            list_node->prev = ref_at_index;
            ref_at_index->next = list_node;
        }
    }
    return list;
}

dplist_t * dpl_remove_at_index( dplist_t * list, int index, bool free_element)
{
    DPLIST_ERR_HANDLER(list==NULL,DPLIST_INVALID_ERROR);
    if(list->head == NULL)
    {// if the list is empty
        return list;
    }
    else if(index <= 0)
    {// if the list is not empty, remove the element in the front of that list

        if(dpl_size(list)!=1) {
            if(free_element)
                list->element_free(&(list->head->element));
            list->head = list->head->next;
            free(list->head->prev);
            list->head->prev = NULL;
        }
        else{
            if(free_element)
                list->element_free(&(list->head->element));
            free(list->head);
            list->head = NULL;
        }
    }
    else
    {// if the list is not empty, remove the element in the middle or the end of that list
        dplist_node_t *ref_at_index = dpl_get_reference_at_index(list,index);
        assert(ref_at_index!=NULL);
        if((index+1)<dpl_size(list))
        {// if the list is not empty, remove the element in the middle of that list
            ref_at_index->prev->next = ref_at_index->next;
            ref_at_index->next->prev = ref_at_index->prev;
            if(free_element)
                list->element_free(&(ref_at_index->element));
            free(ref_at_index);
            ref_at_index = NULL;   //?????????
        }
        else
        {// if the list is not empty, remove the element in the end of that list
            assert(ref_at_index->next == NULL);
            if(ref_at_index->prev != NULL)
            {
                ref_at_index->prev->next = NULL;
                if(free_element)
                    list->element_free(&(ref_at_index->element));
                free(ref_at_index);
                ref_at_index = NULL;   //?????????
            }
            else
            {
                if(free_element)
                    list->element_free(&(list->head->element));
                free(list->head);
                list->head=NULL;
            }
        }
    }
    return list;
}

int dpl_size( dplist_t * list )
{
    DPLIST_ERR_HANDLER(list==NULL,DPLIST_INVALID_ERROR);
    if(list->head == NULL) return 0;
    else {
        int count = 0;
        dplist_node_t *temp_node = list->head;
        while (temp_node) {
            count++;
            if (temp_node->next == NULL) {
                break;
            }
            temp_node = temp_node->next;
        }
        return count;
    }
}

void * dpl_get_element_at_index( dplist_t * list, int index )
{
    // add your code here
    DPLIST_ERR_HANDLER(list==NULL,DPLIST_INVALID_ERROR);
    if(list->head == NULL)
    {
        return (void*) 0;
    }
//    else if(index <= 0)  //not necessary
//    {
//        return list->head->element;
//    }
    else
    {
        int count;
        dplist_node_t *temp_node;
        for (temp_node = list->head, count = 0; temp_node->next != NULL; temp_node = temp_node->next, count++)
        {
            if(count >= index)
            {
                return temp_node->element;
            }
        }
        return temp_node->element;
    }
}

int dpl_get_index_of_element( dplist_t * list, void * element )
{
    // add your code here
    DPLIST_ERR_HANDLER(list==NULL,DPLIST_INVALID_ERROR);
    if (list->head == NULL ) return -1;
    else {
        int count;
        dplist_node_t *dummy;
        for ( dummy = list->head, count = 0; dummy!= NULL ; dummy = dummy->next, count++)
        {
            if (list->element_compare(dummy->element, element) == 1)
                return count;
        }
        // if (list->element_compare(dummy->element, element) == 0)
        //     return count;
        return -1;
    }
}

dplist_node_t * dpl_get_reference_at_index( dplist_t * list, int index )
{
    // add your code here
    DPLIST_ERR_HANDLER(list==NULL,DPLIST_INVALID_ERROR);
    if (list->head == NULL ) return NULL;
//    else if(index <= 0)  //This is not necessary
//    {
//        return list->head;
//    }
    else{
        int count;
        dplist_node_t * dummy;
        for ( dummy = list->head, count = 0; dummy->next != NULL ; dummy = dummy->next, count++)
        {
            if (count >= index)
                return dummy;
        }
        return dummy;
    }
}
 
void * dpl_get_element_at_reference( dplist_t * list, dplist_node_t * reference )
{
    // add your code here
    DPLIST_ERR_HANDLER(list==NULL,DPLIST_INVALID_ERROR);
    if (list->head == NULL ) return NULL;
    else if(reference == NULL)
    {
        dplist_node_t *temp;
        temp = list->head;
        while (temp->next)
        {
            temp = temp->next;
        }
        return temp->element;
    }
    else
    {
        dplist_node_t *dummy;
        for ( dummy = list->head; dummy->next != NULL ; dummy = dummy->next)
        {
            if (dummy == reference)
                return dummy->element;
        }
        if (dummy == reference)
            return dummy->element;
        return NULL;
    }
}


