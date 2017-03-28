#!/usr/bin/python

def partition(a, left, right):
    while left < right:
        if a[left] > a[right]:
            a[left], a[right] = a[right], a[left]
            right -= 1
        left += 1
    return a, right

def sort(a, left, right):
    a, mid = partition(a, left, right)
    if left < mid - 1:
        a = sort(a, left, mid - 1)
    if mid < right:
        a = sort(a, mid, right)
    return a

def test(l):
    print 'before:', l
    l = sort(l, 0, len(l) - 1)
    print 'after: ', l

def main():
    test([1])
    print '--'
    test([2, 1])
    print '--'
    test([2, 1, 3])
    print '--'
    test([9, 3, 2, 3, 99, 2])
    print '--'
    test([9, 9, 9, 9, 3])
    print '--'
    test([1, 2, 3, 4, 5])
    print '--'
    test([5, 4, 3, 2, 1])
    print '--'
    test([6, 5, 4, 3, 2, 1])

main()
