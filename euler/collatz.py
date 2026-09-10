"""
Longest Collatz sequence

The following iterative sequence is defined for the set of positive integers:
n → n/2 (n is even)n → 3n + 1 (n is odd)
Using the rule above and starting with 13, we generate the following sequence:
13 → 40 → 20 → 10 → 5 → 16 → 8 → 4 → 2 → 1
It can be seen that this sequence (starting at 13 and finishing at 1) contains 10 terms. Although it has not been proved yet (Collatz Problem), it is thought that all starting numbers finish at 1.
Which starting number, under one million, produces the longest chain?
NOTE: Once the chain starts the terms are allowed to go above one million.
"""
def collatz(n):
    if n % 2 == 0:
        return n // 2
    return 3*n + 1

class CollatzChain:
    def __init__(self, n):
        self.n = n

    def __iter__(self):
        self.n = self.n
        return self

    def __next__(self):
        if self.n > 1:
            next = collatz(self.n)
            self.n = next
            return next
        else:
            raise StopIteration


def get_chain_length(n, cache):
    if n in cache:
        return cache[n]

    i = 1 
    for x in CollatzChain(n):
        if x in cache:
            length = i + cache[x]
            return length 
        i += 1
    return i


c = {}
greatest_l = 0
greatest_start = 0

for i in range(1000000):
    l = get_chain_length(i, c)
    if l >= greatest_l:
        greatest_start = i
        greatest_l = l
    c[i] = l

# print([(x, c[x]) for x in range(1000)])
print(greatest_start, greatest_l)

# print(get_chain_length(837799, c))

