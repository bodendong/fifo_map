# fifo_map

C语言实现的先进先出散列表，该散列表可以存放用户自定义的数据类型。
可设置表的大小，如果表已存满，再添加数据时，表中最先添加的用户数据会被挤出，即淘汰规则是先进先出。
该表适合查找数据频繁，添加和删除数据相对较少的情况。表的添加、删除、查找操作是线程安全的。
该表在初始化时预先申请所有需要的内存，hash桶的大小在初始化时设置，用户数据key和比较函数也是根据需要自己实现。
