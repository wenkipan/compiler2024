=====

int main()
{
    int i = 0, j = 0;
    int sum = 0;
    int n = 50;
    while(i < 100){
        while(j < n)
        {
            j = j + 1;
            n = n + 4;
            sum = sum + i + j;
        }
        i = i + 1;
    }
    return sum;
}
=====
Segmentation fault (core dumped)

> run build/SysYParser-debug
int main(){
int i=0,j=0;
int sum=0;
int n=50; 
while(i<100){
while(j<n){
j=j+1;
n=n+4;
sum=sum+i+j;
}
i=i+1;
}
return sum;
}
=== program block start ===
ast_stmt_assign
ast_exp_ptr
ast_exp_num
ast_stmt_assign
ast_exp_ptr
ast_exp_num
ast_stmt_assign
ast_exp_ptr
ast_exp_num
ast_stmt_assign
ast_exp_ptr
ast_exp_num
ast_stmt_while
ast_exp_relational
ast_stmt_block
ast_stmt_while
ast_exp_relational
ast_stmt_block
ast_stmt_assign
ast_exp_ptr
ast_exp_binary
ast_stmt_assign
ast_exp_ptr
ast_exp_binary
ast_stmt_assign
ast_exp_ptr
ast_exp_binary
ast_stmt_assign
ast_exp_ptr
ast_exp_binary
ast_stmt_return
ast_exp_load
ast_stmt_return
ast_exp_num


int test(int j[],int i){
    return i+j[2];
}
int main(){
    int i=0;
    int a[10];
    int c=test(a,i);
    return 0;
}


int main()
{
    int i = 0, j=1;
    int sum = 0;
    int n = 50;
    int a[100];
    return sum;
}
int main()
{
    int i;
    int a[10];
    return 0;
}