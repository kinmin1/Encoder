/*
* piclist.c
*
*  Created on: 2015-10-29
*      Author: adminster
*/

#include"piclist.h"
#include "frame.h"
#include "common.h"
#include <stdio.h>
void PicList_pushFront(struct PicList *piclist, struct Frame *curFrame)
{
	//if(!(!curFrame->m_next && !curFrame->m_prev))
	//	printf("piclist: picture already in list\n"); // ensure frame is not in a list
	curFrame->m_next = piclist->m_start;
	curFrame->m_prev = NULL;

	if (piclist->m_count)
	{
		piclist->m_start->m_prev = curFrame;
		piclist->m_start = curFrame;
	}
	else
	{
		piclist->m_start = piclist->m_end = curFrame;
	}
	piclist->m_count++;
}

void PicList_pushBack(struct PicList *piclist, struct Frame *curFrame)
{
	if (!curFrame->m_next && !curFrame->m_prev)
		printf("piclist: picture already in list\n"); // ensure frame is not in a list
	curFrame->m_next = NULL;
	curFrame->m_prev = piclist->m_end;

	if (piclist->m_count)
	{
		piclist->m_end->m_next = curFrame;
		piclist->m_end = curFrame;
	}
	else
	{
		piclist->m_start = piclist->m_end = curFrame;
	}
	piclist->m_count++;
}

struct Frame *PicList_popFront(struct PicList *piclist)
{
	if (piclist->m_start)
	{
		struct Frame *temp = piclist->m_start;
		piclist->m_count--;

		if (piclist->m_count)
		{
			piclist->m_start = piclist->m_start->m_next;
			piclist->m_start->m_prev = NULL;
		}
		else
		{
			piclist->m_start = piclist->m_end = NULL;
		}
		temp->m_next = temp->m_prev = NULL;
		return temp;
	}
	else
		return NULL;
}


struct Frame* PicList_getPOC(struct PicList *piclist, int poc)
{
	struct Frame *curFrame = piclist->m_start;
	while (curFrame && curFrame->m_poc != poc)
		curFrame = curFrame->m_next;
	return curFrame;
}

struct Frame *PicList_popBack(struct PicList *piclist)
{
	if (piclist->m_end)
	{
		struct Frame* temp = piclist->m_end;
		piclist->m_count--;

		if (piclist->m_count)
		{
			piclist->m_end = piclist->m_end->m_prev;
			piclist->m_end->m_next = NULL;
		}
		else
		{
			piclist->m_start = piclist->m_end = NULL;
		}
		temp->m_next = temp->m_prev = NULL;
		return temp;
	}
	else
		return NULL;
}

void PicList_remove(struct PicList *piclist, struct Frame* curFrame)
{
#if _DEBUG
	struct Frame *tmp = piclist->m_start;
	while (tmp && tmp != curFrame)
	{
		tmp = tmp->m_next;
	}

	if (tmp == curFrame)
		printf("piclist: pic being removed was not in list\n"); // verify pic is in this list
#endif

	piclist->m_count--;
	if (piclist->m_count)
	{
		if (piclist->m_start == curFrame)
			piclist->m_start = curFrame->m_next;
		if (piclist->m_end == curFrame)
			piclist->m_end = curFrame->m_prev;

		if (curFrame->m_next)
			curFrame->m_next->m_prev = curFrame->m_prev;
		if (curFrame->m_prev)
			curFrame->m_prev->m_next = curFrame->m_next;
	}
	else
	{
		piclist->m_start = piclist->m_end = NULL;
	}

	curFrame->m_next = curFrame->m_prev = NULL;
}

struct Frame* first(struct PicList *piclist)
{
	return piclist->m_start;
}

struct Frame* last(struct PicList *piclist)
{
	return piclist->m_end;
}

int size(struct PicList *piclist)
{
	return piclist->m_count;
}

int empty(struct PicList *piclist)
{
	return !piclist->m_count;
}

