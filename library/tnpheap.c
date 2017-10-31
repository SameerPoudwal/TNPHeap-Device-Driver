#include <npheap/tnpheap_ioctl.h>
#include <npheap/npheap.h>
#include <npheap.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <signal.h>
#include <malloc.h>
#include <string.h>

struct bufferNode {
    __u64 objectId;
    __u64 size;
    void* addr;
    __u64 version;
    struct bufferNode* next;
};
struct bufferNode *buffer_head;

//DEFINE_MUTEX(global_lock);

__u64 tnpheap_get_version(int npheap_dev, int tnpheap_dev, __u64 offset)
{
    struct tnpheap_cmd cmd;
    cmd.offset = offset;
    struct bufferNode *temp = buffer_head;
    while(temp!=NULL){
        if(temp->objectId==offset){
            return temp->version;
        }
    }
    fprintf(stderr,"No local version number match found");
    // return ioctl(tnpheap_dev,TNPHEAP_IOCTL_GET_VERSION,&cmd);

    struct bufferNode *newNode;
    newNode = (struct bufferNode *)malloc(sizeof(struct bufferNode));
    temp = buffer_head;
    newNode->objectId = offset;
    newNode->size = 0;
    newNode->addr = NULL;
    newNode->version = ioctl(tnpheap_dev, TNPHEAP_IOCTL_GET_VERSION,&cmd);
    newNode->next = NULL;
    if(buffer_head == NULL){
        buffer_head = newNode;
        return buffer_head->addr;
    }
    else{ 
        while(temp->next != NULL)
        {
            temp = temp->next;
        }
        temp->next = newNode;
    }    
    temp = temp->next;
    //list_add(&(newNode->list), &(kernel_llist.list));
    return temp->addr;
}



int tnpheap_handler(int sig, siginfo_t *si)
{
    return 0;
}


void *tnpheap_alloc(int npheap_dev, int tnpheap_dev, __u64 offset, __u64 size)
{
    struct tnpheap_cmd cmd;
  
    struct bufferNode *temp;
    temp = buffer_head;

    while(temp!=NULL){
        if(temp->objectId == offset){
            if(temp->size != size){
                free(temp->addr);
                temp->size = size;
                temp->addr = malloc(size);
                return temp->addr;
            }
            else{
                return temp->addr;
            }
        }
        temp = temp->next;
    }

    struct bufferNode *newNode;
    newNode = (struct bufferNode *)malloc(sizeof(struct bufferNode));
    temp = buffer_head;
    newNode->objectId = offset;
    newNode->size = size;
    newNode->addr = malloc(size);
    newNode->version = ioctl(tnpheap_dev, TNPHEAP_IOCTL_GET_VERSION,&cmd);
    newNode->next = NULL;
    if(buffer_head == NULL){
        buffer_head = newNode;
    }
    else{ 
        while(temp->next != NULL)
        {
            temp = temp->next;
        }
        temp->next = newNode;
    }  
    temp = temp->next;  
    //list_add(&(newNode->list), &(kernel_llist.list));
    return temp->addr;      
}

__u64 tnpheap_start_tx(int npheap_dev, int tnpheap_dev)
{
    struct tnpheap_cmd cmd;

    return ioctl(tnpheap_dev, TNPHEAP_IOCTL_START_TX, &cmd);
}

int tnpheap_commit(int npheap_dev, int tnpheap_dev)
{
    npheap_lock(npheap_dev,10);
    struct tnpheap_cmd cmd;
    struct bufferNode *temp = buffer_head;
    while(temp->next!=NULL){
        cmd.offset = temp->objectId;
        __u64 currentVersion = ioctl(tnpheap_dev,TNPHEAP_IOCTL_GET_VERSION,&cmd);
        if(currentVersion != temp->version){
            list_free();
            return 1;
        }
        temp = temp->next;
    }
    temp = buffer_head;
    while(temp!=NULL){
        cmd.offset = temp->objectId;
        if(ioctl(tnpheap_dev,TNPHEAP_IOCTL_COMMIT, &cmd)==(__u64)0){
            void *mapped_data = npheap_alloc(npheap_dev,temp->objectId,temp->size);
            sprintf(mapped_data,"%s",temp->addr);
        }
        temp = temp->next;
    }
    list_free();
    npheap_unlock(npheap_dev,10);
    return 0;
}

void list_free(){
    struct bufferNode *temp;
    struct bufferNode *temp1;
    temp = buffer_head;

    while(temp!=NULL){
        temp1 = temp->next;
        free(temp->addr);
        free(temp);
        temp = temp1;
    }
}