struct PllLimits {  
    struct {  
        int min, max;  
    } dot, vco, n, m, m1, m2, p, p1;  
    struct {  
        int dot_limit;  
        int slow, fast;  
    } p2;  
};  
  
static const PllLimits limitsG45 = {  
    { 25'000, 270'000 },     
    { 1'750'000, 3'500'000 },   
    { 1, 4 },           
    { 104, 138 },        
    { 17, 23 },          
    { 5, 11 },           
    { 10, 30 },        
    { 1, 3 },         
    { 270'000, 10, 10 }  
};  

struct PllParams { 
    int (*computeDot)(int refclock);
    int (*computeVco)(int refclock); 
    int (*computeM)();
    int (*computeP)();
    
    void (*dump)(int refclock); 
        
    int n, m1, m2, p1, p2;  
};

int PllParams_computeDot(int refclock) {   
    int p = PllParams_computeP();  
    return (PllParams_computeVco(refclock) + p / 2) / p;  
}
        
int PllParams_computeVco(int refclock) {  
    int m = PllParams_computeM();  
    return (refclock * m + (n + 2) / 2) / (n + 2);  
}
     
int PllParams_computeM() {  
    return 5 * (m1 + 2) + (m2 + 2);  
}
     
int PllParams_computeP() {  
    return p1 * p2;  
}
         
void PllParams_dump(int refclock) {
   printf("n: %d, m1: %d, m2: %d, p1: %d, p2: %d\n", n, m1, m2, p1, p2);  
   printf("m: %d, p: %d\n", PllParams_computeM(), PllParams_computeP());  
   printf("dot: %d, vco: %d\n", PllParams_computeDot(refclock), PllParams_computeVco(refclock));  
}  

// 其他结构体Timings、Mode和Framebuffer使用相同方式定义内部函数

struct Timings {  
    int active;  
    int syncStart;  
    int syncEnd;  
    int total;  
    
    int (*blankingStart)(); 
    
    int (*blankingEnd)();

    void (*dump)(); 
}; 

struct Mode {  
    // Desired pixel clock in kHz.  
    int dot;  
    Timings horizontal;  
    Timings vertical;  
};  

struct Framebuffer {  
   unsigned int width, height;  
   unsigned int stride;  
   uintptr_t address;  
};
