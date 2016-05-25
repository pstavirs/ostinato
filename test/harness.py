#! /usr/bin/env python

class Test:
    pass

class TestSuite:
    def __init__(self):
        self.results = []
        self.total = 0
        self.passed = 0
        self.completed = False

    def test_begin(self, name):
        test = Test()
        test.name = name
        test.passed = False
        self.running = test
        print('-----------------------------------------------------------')
        print('@@TEST: %s' % name)
        print('-----------------------------------------------------------')

    def test_end(self, result):
        if self.running:
            self.running.passed = result
            self.results.append(self.running)
            self.total = self.total + 1
            if result:
                self.passed = self.passed + 1
            self.running = None
            print('@@RESULT: %s' % ('PASS' if result else 'FAIL'))
        else:
            raise Exception('Test end without a test begin')

    def report(self):
        print('===========================================================')
        print('TEST REPORT')
        print('===========================================================')
        for test in self.results:
            print('%d: %s' % (test.passed, test.name))
        print('Passed: %d/%d' % (self.passed, self.total))
        print('Completed: %d' % (self.completed))

    def complete(self):
        self.completed = True

    def passed(self):
        return passed == total and self.completed

class TestPreRequisiteError(Exception):
    def __init__(self, msg):
        self.msg = msg
    def __str__(self):
        return self.msg

def extract_column(text, col):
    """Given a text table, return items in the specified column as a list"""
    lines = text.splitlines()
    cols = []
    for line in lines:
        cols.append(line.split(None, col)[col-1])
    return cols
