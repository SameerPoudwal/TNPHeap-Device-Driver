// Project 2: Animesh Sinsinwal, assinsin; Sameer Poudwal, spoudwa; Sayali Godbole, ssgodbol; 

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
    struct bufferNode *next;
};
struct bufferNode *buffer_head = NULL;

//DEFINE_MUTEX(global_lock);

__u64 tnpheap_get_version(int npheap_dev, int tnpheap_dev, __u64 offset)
{
    fprintf(stderr,"into lib get version \n");
    struct tnpheap_cmd cmd;
    cmd.offset = offset;
    struct bufferNode *temp = buffer_head;
    return ioctl(tnpheap_dev, TNPHEAP_IOCTL_GET_VERSION, &cmd);
}



int tnpheap_handler(int sig, siginfo_t *si)
{
    return 0;
}


void *tnpheap_alloc(int npheap_dev, int tnpheap_dev, __u64 offset, __u64 size)
{
    fprintf(stderr,"into lib alloc \n");
    struct tnpheap_cmd cmd;
    cmd.offset = offset;

    struct bufferNode *temp;
    temp = buffer_head;

    while(temp!=NULL){
        if(temp->objectId == offset){
            if(temp->size != size){
                free(temp->addr);
                temp->size = size;
                temp->addr = (void *)malloc(size);   
            }
            return temp->addr;
        }
        temp = temp->next;
    }

    struct bufferNode *newNode;
    newNode = (struct bufferNode *)malloc(sizeof(struct bufferNode));
    temp = buffer_head;
    newNode->objectId = offset;
    newNode->size = size;
    newNode->addr = (void *)malloc(size);
    newNode->version = tnpheap_get_version(npheap_dev, tnpheap_dev, offset);
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
    return temp->addr;      
}

__u64 tnpheap_start_tx(int npheap_dev, int tnpheap_dev)
{
    struct tnpheap_cmd cmd;
    // fprintf(stderr,"into lib start tx \n");
    printf("%s : %d \n", __FILE__,__LINE__);
    return ioctl(tnpheap_dev, TNPHEAP_IOCTL_START_TX, &cmd);
}

int tnpheap_commit(int npheap_dev, int tnpheap_dev)
{
    fprintf(stderr,"into lib commit \n");
    struct tnpheap_cmd cmd;
    __u64 currentVersion;
    __u64 commit_check;
    struct bufferNode *temp = buffer_head;

    if(buffer_head == NULL){
        return 1;
    }

    while(temp!=NULL){
        cmd.offset = temp->objectId;
        // currentVersion = tnpheap_get_version(npheap_dev, tnpheap_dev, cmd.offset);
        currentVersion = ioctl(tnpheap_dev,TNPHEAP_IOCTL_GET_VERSION,&cmd);
        fprintf(stderr, "Comparing versions \n");
        if(currentVersion != temp->version){
            list_free();
            return 1;
        }
        temp = temp->next;
    }
    fprintf(stderr, "all versions match \n")

    //implementing lock
    // npheap_lock(npheap_dev,temp->objectId);
    // npheap_lock(npheap_dev,buffer_head->objectId);
    temp = buffer_head;
    while(temp!=NULL){
        cmd.offset = temp->objectId;
        commit_check = ioctl(tnpheap_dev,TNPHEAP_IOCTL_COMMIT,&cmd);
        if(commit_check!=0){
            fprintf(stderr,"Failed while commiting \n");
            list_free();
            return 1;
        }

        void *mapped_data = (void *)npheap_alloc(npheap_dev, temp->objectId, temp->size);
        memset (mapped_data, 0, temp->size);
        memcpy(mapped_data, temp->addr, temp->size);
        temp = temp->next;
    }
    // npheap_unlock(npheap_dev, temp->objectId);
    // npheap_unlock(npheap_dev, buffer_head->objectId);
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