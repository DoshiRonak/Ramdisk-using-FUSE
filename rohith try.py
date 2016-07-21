'''
a = ['Ronak','Master',['abc','def']]
b = ['Rohith','Master',['ghi','jkl']]
c = ['Ronak','Slave',['mno','pqr']]
d = ['Rohith','Slave',['stu','vwx']]
e = ['Ronak','Master',['xyz','123']]
f = ['Rohith','Slave',['456','789']]
'''
dict = {}

with open('input.txt','r') as f:
    content = f.readlines()


for line in content:
    datasplit = line.split(',')
    #print datasplit
    if datasplit[0] not in dict.keys():
        values = [[],[]]
        if datasplit[1] == 'Master':
            templist = [datasplit[1],[datasplit[2]]]
            values[0] = templist
        else:
            templist = [datasplit[1],[datasplit[2]]]
            values[1] = templist
        dict[datasplit[0]] = values
        #print dict[datasplit[0]]
    else:
        values = dict[datasplit[0]]

        if datasplit[1] == 'Master':
            templist = values[0]
            if len(templist) > 0:
                a=[]
                a.append(datasplit[2])
                #print a
                #print templist[1] 
                templist1 = templist[1] + a
                templist2 = [[templist[0], templist1],values[1]]
                #print templist2
            else:
                templist = [datasplit[1],[datasplit[2]]]
                templist2 = [templist,values[1]]
                
        else:
            templist = values[1]
            if len(templist) > 0:
                a=[]
                a.append(datasplit[2])
                #print a
                #print templist[1]
                templist1 = templist[1] + a
                templist2 = [values[0],[templist[0], templist1]]
            else:
                templist = [datasplit[1],[datasplit[2]]]
                templist2 = [values[0],templist]
                
        dict[datasplit[0]] = templist2
        #print dict[datasplit[0]]
        
for k,v in dict.items():
    print k,v






'''
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
'''
