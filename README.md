---====== * ======---

one directory file system
- project name : odfs Costa Rica
- on the surface it is merely revolutionary file system that allows to flexible multi-categorization of resources
- at second glance is it obvious that it is best yet file system to re-create human brain behavior,
  also solve and save all world's problems in file system fields

---====== * ======---

odfs costa rica spec.
- POSIX compatibility
- uniq file identyfication : numeric id
- odfs allows to use tags, that can be use to add aditional information about id
- odfs's tags categories : generic tags , like name, size, date
                           user tag , any new tag added by user , like projects, importatnt, temporary
                           plugin tags , tags that will be added by plugin, i.e. tags extracted from mp3 metadata by mp3_odfs_plugin, like bitrate, author, length, ...
  tagnames must be uniq so maybe prefix according category should be used, like tag : tamporary, user.temporary, pluginname.temporary
- odfs stores filesystem data in key/value database, and relays on its speed
- odfs provide way to use tagnames to list files

---====== * ======---

special use, reduplication:
- odfs may be usefull to organize vast file based dumps. Usually users collections of files my contain repetitive data, because of
  different names of the same album, different file encodding, different file schema ( album long file, or single track long files ),
  and so on ...
  Compare to other application which allow to find duplicates odfs :
  - use more sophisticated categories ( other application tend to use only name, file type, md5 checksum )
  - use metadata so searches are quicker
  - allow to easy customisation by plugin mechanism to process generation of requested new tags
  - ...

---====== * ======---

