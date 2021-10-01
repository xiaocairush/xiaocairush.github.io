#include <iostream>
#include <stack>
using namespace std;
#define INF 0x7fffffff

stack<int> todolist;
long long a[80005];
int main(int argc, const char * argv[]) {
    int n;
    scanf("%d", &n);
    long long ans= 0;
    for (int i = 0; i <= n; i++) {
        if(i < n)
            scanf("%lld", &a[i]);
        else
            a[i] = INF;
        while (!todolist.empty() && a[i] >= a[todolist.top()]) {
            ans += i - todolist.top() - 1;
            todolist.pop();
        }
        todolist.push(i);
    }
    printf("%lld\n", ans);
    return 0;
}
