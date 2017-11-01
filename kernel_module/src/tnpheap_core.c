// 
//////////////////////////////////////////////////////////////////////
//                             North Carolina State University
//
//
//
//                             Copyright 2016
//
////////////////////////////////////////////////////////////////////////
//
// This program is free software; you can redistribute it and/or modify it
// under the terms and conditions of the GNU General Public License,
// version 2, as published by the Free Software Foundation.
//
// This program is distributed in the hope it will be useful, but WITHOUT
// ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
// FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
// more details.
//
// You should have received a copy of the GNU General Public License along with
// this program; if not, write to the Free Software Foundation, Inc.,
// 51 Franklin St - Fifth Floor, Boston, MA 02110-1301 USA.
//
////////////////////////////////////////////////////////////////////////
//
//   Author:  Hung-Wei Tseng
//
//   Description:
//     Skeleton of NPHeap Pseudo Device
//
////////////////////////////////////////////////////////////////////////

#include "tnpheap_ioctl.h"

#include <asm/uaccess.h>
#include <linux/slab.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/mm.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/poll.h>
#include <linux/mutex.h>
#include <linux/time.h>

struct miscdevice tnpheap_dev;
struct node kernel_llist;
struct node {
    __u64 objectId;
    __u64 size;
  //  void* k_virtual_addr;
    __u64 versionNo;
    struct list_head list;
};

struct mutex list_lock;

__u64 transaction_id = 0;

__u64 tnpheap_get_version(struct tnpheap_cmd __user *user_cmd)
{
    struct tnpheap_cmd cmd;
    printk("Get Version called");
    if (copy_from_user(&cmd, user_cmd, sizeof(cmd))==0)
    {
        struct list_head *position;
        struct node *llist;
        struct node *newNode;
        printk("Traversing Linked List");
        list_for_each(position, &kernel_llist.list){
           llist = list_entry(position, struct node, list);
           if(llist->objectId == (__u64)cmd.offset)
           {
               printk("Object found");
               return llist->versionNo;
           }
        }
        printk("Creating new Object");
        newNode = (struct node *)kmalloc(sizeof(struct node), GFP_KERNEL);
        newNode->objectId = cmd.offset;
        newNode->size = cmd.size;
        newNode->versionNo = (__u64)0;
        list_add(&(newNode->list), &(kernel_llist.list));
        return newNode->versionNo;
    } 
    return (__u64)-1;
}

__u64 tnpheap_start_tx(struct tnpheap_cmd __user *user_cmd)
{
    struct tnpheap_cmd cmd;
    //__u64 ret=0;
    if (copy_from_user(&cmd, user_cmd, sizeof(cmd)))
    {
        return (__u64)-1 ;
    }
    mutex_lock(&list_lock);
    transaction_id++;
    mutex_unlock(&list_lock);
    return transaction_id;
}

__u64 tnpheap_commit(struct tnpheap_cmd __user *user_cmd)
{
    struct tnpheap_cmd cmd;
    struct node *newNode;
    __u64 ret=0;

    if (copy_from_user(&cmd, user_cmd, sizeof(cmd))==0)
    {
        struct list_head *position;
        struct node *llist;
        printk("Traversing Linked List");
        
        list_for_each(position, &kernel_llist.list){
           llist = list_entry(position, struct node, list);
           if(llist->objectId == (__u64)cmd.offset){


               if(llist->versionNo == cmd.version ){
                    newNode = (struct node *)kmalloc(sizeof(struct node), GFP_KERNEL);
                    newNode->objectId = cmd.offset;
                    newNode->size = cmd.size;
                    newNode->versionNo = llist->versionNo + 1;
                    list_replace(llist,newNode);
                    kfree(llist);
                    return (__u64)0;
               }else{
                    printk("Version numbers dont match in kernel space");
                    //mutex_unlock(&list_lock);
                    return (__u64)1;
            }    
           }
           /*else{
               newNode = (struct node *)kmalloc(sizeof(struct node), GFP_KERNEL);
               newNode->objectId = cmd.offset;
               newNode->size = cmd.size;
               newNode->versionNo = 0;
               list_add(&(newNode->list), &(kernel_llist.list));
               ret = 0;
           }*/
        }
    }
    return (__u64)-1;
}



__u64 tnpheap_ioctl(struct file *filp, unsigned int cmd,
                                unsigned long arg)
{
    switch (cmd) {
    case TNPHEAP_IOCTL_START_TX:
        return tnpheap_start_tx((void __user *) arg);
    case TNPHEAP_IOCTL_GET_VERSION:
        return tnpheap_get_version((void __user *) arg);
    case TNPHEAP_IOCTL_COMMIT:
        return tnpheap_commit((void __user *) arg);
    default:
        return -ENOTTY;
    }
}

static const struct file_operations tnpheap_fops = {
    .owner                = THIS_MODULE,
    .unlocked_ioctl       = tnpheap_ioctl,
};

struct miscdevice tnpheap_dev = {
    .minor = MISC_DYNAMIC_MINOR,
    .name = "tnpheap",
    .fops = &tnpheap_fops,
};

static int __init tnpheap_module_init(void)
{
    int ret = 0;
    if ((ret = misc_register(&tnpheap_dev)))
        printk(KERN_ERR "Unable to register \"npheap\" misc device\n");
    else{
        printk(KERN_ERR "\"npheap\" misc device installed\n");
        //Initializing kernel head list.
        INIT_LIST_HEAD(&kernel_llist.list);
        //Initializing GLOBAL LOCK
        mutex_init(&list_lock);
    }

    return 1;
}

static void __exit tnpheap_module_exit(void)
{
    misc_deregister(&tnpheap_dev);
    return;
}

MODULE_AUTHOR("Hung-Wei Tseng <htseng3@ncsu.edu>");
MODULE_LICENSE("GPL");
MODULE_VERSION("0.1");
module_init(tnpheap_module_init);
module_exit(tnpheap_module_exit);