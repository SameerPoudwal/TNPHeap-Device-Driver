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
    return -1;
}



int tnpheap_handler(int sig, siginfo_t *si)
{
    return 0;
}


void *tnpheap_alloc(int npheap_dev, int tnpheap_dev, __u64 offset, __u64 size)
{
    struct tnpheap_cmd cmd;
    struct bufferNode *newNode;
    newNode = (struct bufferNode *)malloc(sizeof(struct bufferNode));
    struct bufferNode *temp;
    temp = buffer_head;
    while(temp!=NULL){
        if(temp->objectId == offset){
            free(temp->addr);
            temp->size = size;
            temp->addr = malloc(size);
            return temp->addr;
        }
        temp = temp->next;
    }
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
    //list_add(&(newNode->list), &(kernel_llist.list));
    return newNode->addr;      
}

__u64 tnpheap_start_tx(int npheap_dev, int tnpheap_dev)
{
    struct tnpheap_cmd cmd;
}

int tnpheap_commit(int npheap_dev, int tnpheap_dev)
{
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
            list_free();
        }
        temp = temp->next;
    }
    return 0;
}

void list_free(){
    struct bufferNode *temp;
    struct bufferNode *temp1;
    temp = buffer_head;
    while(temp!=NULL){
        temp1 = temp;
        free(temp);
        temp = temp1->next;
    }
}

// void *getObject(__u64 ObjectId)
// {
//     struct tnpheap_cmd cmd;
//     struct bufferNode *newNode;
//     newNode = (struct bufferNode *)malloc(sizeof(struct bufferNode));
//     struct bufferNode *temp;
//     temp = buffer_head;
//     while(temp->next!=NULL){
//         if(temp->objectId == offset){
//            return temp->addr;
//         }
//         temp = temp->next;
//     }
//     temp = buffer_head;
//     newNode->objectId = offset;
//     newNode->size = size;
//     newNode->addr = malloc(size);
//     newNode->version = ioctl(tnpheap_dev, TNPHEAP_IOCTL_GET_VERSION,&cmd);
//     newNode->next = NULL;
//     if(buffer_head == NULL){
//         buffer_head = newNode;
//     }
//     else{ 
//         while(temp->next != NULL)
//         {
//             temp = temp->next;
//         }
//         temp->next = newNode;
//     }    

//     return newNode->addr;
// }



