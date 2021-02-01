/********************************************************************************/
/*																				*/
/*   이 소스 코드의 권리를 명시하는 주석을 제거하지 마시오.							*/
/*   Copyright 2020 by keh														*/
/*																				*/
/*   by keh																		*/
/********************************************************************************/

/********************************************************************
2018.11.11 by KEH
-------------------------------------------------------------
Quick Sort API
*********************************************************************/

#pragma once
#include	<string.h>

typedef struct {
	unsigned long address;
	void *p;
} STSortData;

template <typename DATATYPE>
class CQeSort
{
public:
	CQeSort();
	~CQeSort();
	void eSort(DATATYPE *arr, int nSize);
	void getMinMax(DATATYPE *arr, int nCnt, int*pMin, int*pMax);
private:
};

void SortEx(STSortData *arr, int nSize);

template <typename DATATYPE>
CQeSort<DATATYPE>::CQeSort()
{
}

template <typename DATATYPE>
CQeSort<DATATYPE>::~CQeSort()
{
}

template <typename DATATYPE>
void CQeSort<DATATYPE>::eSort(DATATYPE *arr, int nSize)
{
	int loopcnt = nSize >> 1, left, right = nSize-1;
	int nMin, nMax;
	int i, j, val1, val2;
	for (i = 0; i < loopcnt; i++)
	{
		left = i;
		//------------------------------------
		nMin = left; nMax = left;
		for (j = left + 1; j <= right; j++) {
			if (arr[nMin] > arr[j])nMin = j;
			else if (arr[nMax] < arr[j])nMax = j;
		}
		if (nMin == nMax) return;

		if (left == nMax) {
			val1 = arr[right];
			val2 = arr[nMax];
			arr[left] = arr[nMin];
			arr[right] = val2;
			arr[nMin] = val1;
		}
		else if (left == nMin) {
			val1 = arr[right];
			arr[right] = arr[nMax];
			arr[nMax] = val1;
		}
		else if (right == nMin) {
			val1 = arr[left];
			val2 = arr[nMin];
			arr[right] = arr[nMax];
			arr[left] = val2;
			arr[nMax] = val1;
		}
		else if (right == nMax) {
			val1 = arr[left];
			arr[left] = arr[nMin];
			arr[nMin] = val1;
		}
		else {
			val1 = arr[left];
			val2 = arr[right];
			arr[right] = arr[nMax];
			arr[left] = arr[nMin];
			arr[nMin] = val1;
			arr[nMax] = val2;
		}

		//-------------------------------------
		right--;
	}

}

template <typename DATATYPE>
void CQeSort<DATATYPE>::getMinMax(DATATYPE *arr, int nCnt, int*pMin, int*pMax)
{
	int nMin=0, nMax=0,i;
	for (i = 1; i < nCnt; i++) {
		if(arr[nMin] > arr[i])nMin = i;
		else if(arr[nMax] < arr[i])nMax = i;
	}
	*pMin = arr[nMin];
	*pMax = arr[nMax];
}

