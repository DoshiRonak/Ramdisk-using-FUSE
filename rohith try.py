dict = {'Ronak':[['Master',['abc.com']],['Slave',['cde.com']]],
        'Rohith':[['Master',['cde.com']],['Slave',['abc.com']]],
        'Chandan':[['Master',['efg.com']]],
        'Shyam':[['Slave',['efg.com']]]
        }

Zones = {'abc.com':['Ronak','Rohith'],'cde.com':['Rohith','Ronak'],'efg.com':['Chandan','Shyam']}

#Ronak, Master, Rohit, abc.com

#print dict, Zones

#print Zones['abc.com'][1]

#for k,v in dict.items():
 #   print v

def fun(a):
    #print a
    ls = []
    if len(a) == 1:
        b = a[0]
        if b[0] == 'Master':
            for val in b[1]:
                ls.append(Zones[val][1])
        else:
            for val in b[1]:
                ls.append(Zones[val][0])
    else:
        value1 = a[0]
        value2 = a[1]
        #print value1, value2
        if value1[0] == 'Master':
            for val in value1[1]:
                ls.append(Zones[val][1])
        else:
            for val in value1[1]:
                ls.append(Zones[val][0])

        if value2[0] == 'Master':
            for val in value2[1]:
                ls.append(Zones[val][1])
        else:
            for val in value2[1]:
                ls.append(Zones[val][0])
    return ls

for k,v in dict.items():
    ls = fun(v)
    print k, ls
