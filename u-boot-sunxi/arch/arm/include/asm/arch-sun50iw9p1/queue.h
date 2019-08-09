/*
 *  * Copyright 2000-2009
 *   * Wolfgang Denk, DENX Software Engineering, wd@denx.de.
 *    *
 *     * SPDX-License-Identifier:	GPL-2.0+
 *     */


/* �����Ա������:��Ϊ��ʽ������˳�����
	˳�����:��һ�ε�ַ�����Ĵ洢��Ԫ�洢����Ԫ�أ����������α�:ָ���ͷ
	���α�(front)��ָ���β���α�(rear),���front == rear����Ϊ��,���
	(rear + 1) % MAXSIZE == front������(��Ϊѭ������),����ͨ����rear==MAXSIZE������
*/

#ifndef __QUEUE_H__
#define __QUEUE_H__


#define QUEUE_MAX_BUFFER_SIZE    64  /* max buffer count of queue  */


typedef struct {
	char *data;
	uint  len;
}
queue_data;

typedef struct {
	queue_data element[QUEUE_MAX_BUFFER_SIZE];
	int front;                     /* head buffer of the queue */
	int rear;                      /* tail buffer of the queue */
    int size;                      /* the bytes of each buffer int the queue */
    int count;                     /* the total count of buffers in the queue */
    void *base_addr;
} queue;

int  initqueue(queue *q, int each_size, int buffer_count);  /* init queue */

int  destroyqueue(queue *q);         /* destroy queue */

void resetqueue(queue *q);

int isqueueempty(queue *q);

int isqueuefull(queue *q);

int inqueue_query(queue *q, queue_data *qdata);

int inqueue_ex(queue *q);

int outqueue_query(queue *q, queue_data *qdata, queue_data *next_qdata);

int outqueue_ex(queue *q);



#endif /* __QUEUE_H__ */


