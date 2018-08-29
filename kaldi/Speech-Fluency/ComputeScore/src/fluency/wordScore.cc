
#include "wordScore.h"

#define Minimum(a,b) ((a<b)?a:b)
/*void WordScore::SplitStringToVector(const std::string strSource, std::vector<std::string>& res){
    std::vector<std::string>::size_type sPos = 0;
    std::vector<std::string>::size_type ePos = strSource.find(" ", sPos);
    while(ePos != std::string::npos){
        if(sPos != ePos) res.push_back(strSource.substr(sPos, ePos - sPos));
        sPos = ePos + 1;
        ePos = strSource.find(" ", sPos);
    }
    if(sPos < strSource.size()) res.push_back(strSource.substr(sPos, strSource.size() - sPos));
}*/

float WordScore::EditDistance(const std::vector<std::string> &v1, const std::vector<std::string> &v2){
	int max1 = v1.size();
  	int max2 = v2.size();

  	int **ptr = new int*[max1 + 1];
	for(int i = 0; i < max1 + 1 ;i++){
	    ptr[i] = new int[max2 + 1];
	}

	for(int i = 0 ;i < max1 + 1 ;i++){
	    ptr[i][0] = i;
	}

	for(int i = 0 ;i < max2 + 1;i++){
	    ptr[0][i] = i;
	}

	for(int i = 1 ;i < max1 + 1 ;i++){
	    for(int j = 1 ;j< max2 + 1; j++){
	        int d;
	        int temp = Minimum(ptr[i-1][j] + 1, ptr[i][j-1] + 1);
	        if((v1.at(i-1)).compare(v2.at(j-1)) == 0){
	            d = 0 ;
	        }
	        else{
	            d = 1 ;
	        }
	        ptr[i][j] = Minimum(temp, ptr[i-1][j-1] + d);
	    }
	}

	int dis = ptr[max1][max2];

	for(int i = 0; i < max1 + 1; i++){
	    delete[] ptr[i];
	    ptr[i] = NULL;
	}

	delete[] ptr;
	ptr = NULL;
	return dis;
}