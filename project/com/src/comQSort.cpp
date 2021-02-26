#include "comQSort.h"


void SortEx(STSortData *arr, int nCount)
{
	int loopcnt = nCount >> 1, left, right = nCount - 1;
	int nMin, nMax;
	int i, j, nSize = sizeof(STSortData);
	STSortData stVal1, stVal2;

	for (i = 0; i < loopcnt; i++)
	{
		left = i;
		//------------------------------------
		nMin = left; nMax = left;
		for (j = left + 1; j <= right; j++) {
			if (arr[nMin].nValue > arr[j].nValue)nMin = j;
			else if (arr[nMax].nValue < arr[j].nValue)nMax = j;
		}
		if (nMin == nMax) return;

		if (left == nMax) {
			memcpy(&stVal2, &arr[nMax], nSize);
			if (right == nMin) {
				memcpy(&arr[left], &arr[nMin], nSize);
				memcpy(&arr[right], &stVal2, nSize);
			}
			else {
				memcpy(&stVal1, &arr[right], nSize);
				memcpy(&arr[left], &arr[nMin], nSize);
				memcpy(&arr[right], &stVal2, nSize);
				memcpy(&arr[nMin], &stVal1, nSize);
			}
		}
		else if (left == nMin) {
			memcpy(&stVal1, &arr[right], nSize);
			memcpy(&arr[right], &arr[nMax], nSize);
			memcpy(&arr[nMax], &stVal1, nSize);
		}
		else if (right == nMin) {
			memcpy(&stVal1, &arr[left], nSize);
			memcpy(&stVal2, &arr[nMin], nSize);
			memcpy(&arr[right], &arr[nMax], nSize);
			memcpy(&arr[left], &stVal2, nSize);
			memcpy(&arr[nMax], &stVal1, nSize);
		}
		else if (right == nMax) {
			memcpy(&stVal1, &arr[left], nSize);
			memcpy(&arr[left], &arr[nMin], nSize);
			memcpy(&arr[nMin], &stVal1, nSize);
		}
		else {
			memcpy(&stVal1, &arr[left], nSize);
			memcpy(&stVal2, &arr[right], nSize);
			memcpy(&arr[right], &arr[nMax], nSize);
			memcpy(&arr[left], &arr[nMin], nSize);
			memcpy(&arr[nMin], &stVal1, nSize);
			memcpy(&arr[nMax], &stVal2, nSize);
		}

		//-------------------------------------
		right--;
	}
}


void getMinMax(STSortData *arr, int nCnt, int*pMin, int*pMax)
{
	int nMin = 0, nMax = 0, i;
	for (i = 1; i < nCnt; i++) {
		if (arr[nMin].nValue > arr[i].nValue) nMin = i;
		else if (arr[nMax].nValue < arr[i].nValue) nMax = i;
	}
	*pMin = arr[nMin].nValue;
	*pMax = arr[nMax].nValue;
}