/*
* piclist.h
*
*  Created on: 2015-10-29
*      Author: adminster
*/

#ifndef PICLIST_H_
#define PICLIST_H_

struct PicList
{
	struct Frame*   m_start;
	struct Frame*   m_end;
	int      m_count;

	struct Frame* (*getPOC)(struct PicList *piclist, int poc);
};

/** Find frame with specified POC */
struct Frame* PicList_getPOC(struct PicList *piclist, int poc);
void PicList_pushFront(struct PicList *piclist, struct Frame *curFrame);
void PicList_pushBack(struct PicList *piclist, struct Frame *curFrame);
struct Frame *PicList_popFront(struct PicList *piclist);
struct Frame *PicList_popBack(struct PicList *piclist);
void PicList_remove(struct PicList *piclist, struct Frame* curFrame);
struct Frame* first(struct PicList *piclist);
struct Frame* last(struct PicList *piclist);
int size(struct PicList *piclist);
int empty(struct PicList *piclist);

#endif /* PICLIST_H_ */
