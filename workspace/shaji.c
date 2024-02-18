#include<stdio.h>
int s[5000];
int main(){
	int a,b,c,d,e,i,j,k,l;
	scanf("%d",&l);
	for(i=0;i<5000;i++){
		s[i]=0;
	}
	for(i=0;i<l;i++){
		s[i]=1;
	}
	k=l;
	i=0;
	j=0;
	for(;;){
		if(i>=l){
			i=0;
		}
		if(k==1){
			for(c=0;c<l;c++){
				if(s[c]==1){
					goto fin;
				}
			}
		}
		if(s[i]==1){
			j++;
			if(j==7){
				printf("kill %d\n",i+1);
				s[i]=0;
				j=0;
				k--;
			}
		}
				i++;
	}
	fin:
	printf("%d\n",c+1);
}