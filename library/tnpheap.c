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
    /*
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
    */
    return ioctl(tnpheap_dev, TNPHEAP_IOCTL_GET_VERSION, &cmd);
}



int tnpheap_handler(int sig, siginfo_t *si)
{
    return 0;
}


void *tnpheap_alloc(int npheap_dev, int tnpheap_dev, __u64 offset, __u64 size)
{
    struct tnpheap_cmd cmd;
    cmd.offset = offset;

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
    newNode->version = tnpheap_get_version(npheap_dev, tnpheap_dev, offset);
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
    
    struct tnpheap_cmd cmd;
    __u64 currentVersion;
    int commit_check;
    struct bufferNode *temp = buffer_head;

    if(buffer_head == NULL){
        return 1;
    }

    while(temp->next!=NULL){
        cmd.offset = temp->objectId;
        currentVersion = ioctl(tnpheap_dev,TNPHEAP_IOCTL_GET_VERSION,&cmd);
        if(currentVersion != temp->version){
            list_free();
            return 1;
        }
        temp = temp->next;
    }

    //implementing lock
    npheap_lock(npheap_dev,temp->objectId);
    temp = buffer_head;
    while(temp!=NULL){
        cmd.offset = temp->objectId;
        commit_check = ioctl(tnpheap_dev,TNPHEAP_IOCTL_COMMIT,&cmd);
        if(commit_check!=0){
            fprintf(stderr,"Failed while commiting\n");
            list_free();
            return 1;
        }

        void *mapped_data = (void *)npheap_alloc(npheap_dev, temp->objectId, temp->size);
        npheap_unlock(npheap_dev, temp->objectId);

        /*
        if(ioctl(tnpheap_dev,TNPHEAP_IOCTL_COMMIT, &cmd)==(__u64)0){
            void *mapped_data = npheap_alloc(npheap_dev,temp->objectId,temp->size);
            sprintf(mapped_data,"%s",temp->addr);
        }*/
        temp = temp->next;
    }
    memcpy(mapped_data, temp->addr, temp->size);
    list_free();
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