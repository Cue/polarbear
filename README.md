Polarbear
=========

A tool to help diagnose OutOfMemoryError conditions.
----------------------------------------------------

Polarbear helps track down the root cause of OutOfMemoryError exceptions in Java.  When the JVM runs out of memory,
polarbear will log detailed information about what threads are running and what objects are live to /tmp/oom.log for
later review.

### Status:

This is a very early stage project.  It works for our needs.  We haven't verified it works beyond that.  Issue reports
and patches are very much appreciated!


### Installation

OS X:

    gnumake J2SDK=/System/Library/Frameworks/JavaVM.framework/Versions/1.6/Home OSNAME=darwin clean test


Ubuntu:

    make J2SDK=/usr/lib/jvm/java-1.6.0-openjdk OSNAME=linux clean test


Running make will build a JVMTI library.  You can enable with a command line flag like the following:

    -agentpath:/path/to/liboutOfMemory.so=ImportantClass,AnotherImportantClass

This will produce the normal output (see Sample Output below) in addition to calculating retained sizes for ImportantClass
and AnotherImportantClass instances.

### polarbear shell


polarbear provides a limited repl on 8787 that allows you to interact directly.

#### Note: 8787 is not secure or fault tolerant and MUST be protected in other ways


```
> telnet localhost 8787

histogram

[...]

stats Lorg/apache/lucene/index/IndexWriter;

[...]
```




### Sample Output

    Exception in thread "main" java.lang.OutOfMemoryError: Java heap space
            at java.lang.StringBuilder.toString(StringBuilder.java:430)
            at Test.main(Test.java:16)
    Initializing polarbear.

    Performing retained size analysis for 2 classes:
    HashMap
    OOMList

    About to throw an OutOfMemory error.
    Suspending all threads except the current one.
    Printing a heap histogram.
    Heap View, Total of 799172 objects found.

    Space      Count      Retained   Class Signature
    ---------- ---------- ---------- ----------------------
      31692904     396154          0 [C
    Collecting references... Finding references to references... Aggregating references...
                    Level 1 referrers:
                        396191 Ljava/lang/String;
                             3 Ljava/lang/Thread;
                             1 Ljava/lang/ThreadLocal$ThreadLocalMap$Entry;
                             1 Ljava/lang/ref/Finalizer$FinalizerThread;
                             1 Ljava/lang/ref/Reference$ReferenceHandler;
                             2 Ljava/io/BufferedWriter;
                             1 Ljava/lang/StringBuilder;

                    Level 2 referrers:
                           313 [Ljava/lang/Object;
                           314 Ljava/util/HashMap$Entry;
                           166 Ljava/util/LinkedHashMap$Entry;
                           309 Ljava/util/Hashtable$Entry;
                           186 Ljava/util/LinkedList$Entry;
                           259 Ljava/lang/Integer;
                             8 Lsun/security/util/ObjectIdentifier;
                            48 Ljava/security/Provider$Service;
                            40 Ljava/net/URL;
                            59 [Ljava/lang/String;
                            93 Ljava/security/Provider$ServiceKey;
                            46 Ljava/security/Provider$UString;
                            31 Ljava/util/concurrent/ConcurrentHashMap$HashEntry;
                            30 Ljava/security/Provider$EngineDescription;
                            16 Ljava/util/jar/JarFile;
                            23 Lsun/security/x509/OIDMap$OIDInfo;
                            28 Ljava/io/ExpiringCache$Entry;
                            16 Ljava/io/ObjectStreamField;
                            19 Ljava/util/Locale;
                            18 Lsun/security/x509/RDN;
                            21 Lsun/security/x509/AVAKeyword;
                             4 Ljava/lang/reflect/Method;
                            17 Ljava/util/jar/Attributes$Name;
                            11 Lsun/security/jca/ProviderConfig;
                             4 Ljava/util/jar/JarFile$JarFileEntry;
                             1 Ljava/lang/ref/WeakReference;
                             4 Ljava/lang/OutOfMemoryError;
                             1 [Ljava/lang/ThreadLocal$ThreadLocalMap$Entry;
                             2 Lsun/security/provider/Sun;
                             3 Lsun/security/x509/X500Name;
                             7 Lsun/security/x509/NetscapeCertTypeExtension$MapEntry;
                             2 Ljava/lang/reflect/Field;
                             2 Lsun/nio/cs/MacRoman$Encoder;
                             2 Lsun/security/x509/X509CertImpl;
                             5 Lsun/security/jca/ServiceId;
                             4 Ljava/io/File;
                             1 Lsun/security/jca/ProviderList$1;
                             2 Ljava/lang/ThreadGroup;
                             2 Ljava/util/Properties;
                             4 Ljava/text/Normalizer$Form;
                             3 Lsun/misc/Signal;
                             3 Ljava/lang/RuntimePermission;
                             2 Ljava/security/AlgorithmParameters;
                             2 Ljava/io/PrintStream;
                             2 [Ljava/lang/Thread;
                             3 Ljava/nio/charset/CodingErrorAction;
                             1 Ljava/io/UnixFileSystem;
                             1 Ljava/lang/ArithmeticException;
                             1 Ljava/lang/ClassLoader$NativeLibrary;
                             1 Ljava/io/FilePermission;
                             1 Lsun/nio/cs/StandardCharsets;
                             2 Ljava/nio/ByteOrder;
                             1 Lcom/apple/java/Usage;
                             1 Ljava/lang/reflect/ReflectPermission;
                             1 Lsun/nio/cs/US_ASCII;
                             1 Lsun/security/provider/certpath/X509CertPath;
                             1 Lsun/nio/cs/UTF_8;
                             1 Lsun/nio/cs/MacRoman;
                             1 Lsun/security/x509/RFC822Name;

                    Level 3 referrers:
                            10 [Ljava/lang/Object;
                            62 [Ljava/util/HashMap$Entry;
                            49 Ljava/util/HashMap$Entry;
                            96 Ljava/util/LinkedHashMap$Entry;
                             6 Ljava/util/Hashtable$Entry;
                           102 Ljava/util/LinkedList$Entry;
                            13 [Ljava/util/Hashtable$Entry;
                            79 Ljava/math/BigInteger;
                             2 Ljava/lang/reflect/Constructor;
                            21 [Ljava/util/concurrent/ConcurrentHashMap$HashEntry;
                             1 [Ljava/lang/Integer;
                            17 Ljava/lang/ref/Finalizer;
                             2 Ljava/util/Vector;
                            23 Lsun/security/util/DerInputBuffer;
                            23 Lsun/security/util/DerValue;
                            16 Lsun/misc/URLClassPath$JarLoader;
                             7 [Ljava/lang/Class;
                            23 Ljava/util/ArrayList;
                            21 Lsun/security/x509/AVA;
                            19 Ljava/lang/Object;
                            16 Ljava/lang/Byte;
                             5 Ljava/lang/ref/WeakReference;
                             6 [Ljava/io/ObjectStreamField;
                             9 Lsun/misc/JarIndex;
                             6 Lsun/security/x509/AlgorithmId;
                             3 [Lsun/security/x509/RDN;
                             2 Lsun/security/x509/X509CertInfo;
                             2 Lsun/security/util/MemoryCache$SoftCacheEntry;
                             2 Lsun/nio/cs/StreamEncoder;
                             6 Ljava/util/concurrent/atomic/AtomicInteger;
                             4 Ljava/util/Date;
                             1 Ljava/util/jar/JarVerifier;
                             1 [Lsun/security/jca/ProviderConfig;
                             1 Lsun/misc/Launcher$ExtClassLoader;
                             2 Lsun/security/x509/AuthorityKeyIdentifierExtension;
                             1 Ljava/lang/ThreadLocal$ThreadLocalMap;
                             1 Lsun/misc/Launcher$AppClassLoader;
                             2 Lsun/misc/URLClassPath;
                             2 Lsun/security/x509/NetscapeCertTypeExtension;
                             2 Ljava/io/OutputStreamWriter;
                             2 Ljava/security/Permissions;
                             2 Lsun/security/x509/SubjectKeyIdentifierExtension;
                             2 Lsun/security/x509/CertificateValidity;
                             1 LTest$OOMList;
                             2 Lsun/security/x509/CertificateIssuerName;
                             2 Ljava/nio/charset/CoderResult;
                             1 [Ljava/io/File;
                             2 Lsun/security/provider/DSAParameters;
                             1 [Lsun/security/x509/NetscapeCertTypeExtension$MapEntry;
                             2 Lsun/security/x509/CertificateExtensions;
                             2 Lsun/security/jca/ProviderList;
                             2 Lsun/security/x509/CertificateSubjectName;
                             1 Ljava/security/ProtectionDomain;
                             1 Ljava/io/BufferedInputStream;
                             1 Lsun/nio/cs/StandardCharsets$Aliases;
                             1 Lsun/nio/cs/StandardCharsets$Cache;
                             1 Ljava/util/IdentityHashMap;
                             1 [Lsun/security/jca/ServiceId;
                             1 Lsun/nio/cs/StandardCharsets$Classes;
                             1 Lsun/security/x509/BasicConstraintsExtension;
                             1 [Ljava/text/Normalizer$Form;
                             1 Lsun/security/x509/KeyUsageExtension;
                             2 Lsun/security/x509/CertificateVersion;
                             2 Lsun/net/www/protocol/jar/Handler;
                             2 Ljava/lang/Boolean;
                             1 Ljava/security/CodeSource;
                             1 [Ljava/lang/ThreadGroup;
                             2 Lsun/security/x509/CertificateSerialNumber;
                             1 Ljava/lang/ref/Reference;
                             1 Lsun/security/x509/SubjectAlternativeNameExtension;
                             2 Lsun/security/x509/CertificateX509Key;
                             1 Ljava/util/concurrent/atomic/AtomicReferenceFieldUpdater$AtomicReferenceFieldUpdaterImpl;
                             2 Lsun/security/x509/CertificateAlgorithmId;
                             1 Ljava/security/BasicPermissionCollection;
                             1 Lsun/misc/URLClassPath$FileLoader;
                             1 Ljava/math/MutableBigInteger;
                             1 Ljava/security/CodeSigner;
                             1 Ljava/io/FilePermissionCollection;
                             1 Ljava/util/BitSet;
                             1 Lsun/misc/Launcher$Factory;
                             1 Lsun/net/www/protocol/file/Handler;
                             1 Ljava/util/jar/JavaUtilJarAccessImpl;
                             1 Lcom/apple/java/Usage$1;
                             1 Lsun/reflect/ReflectionFactory;
                             1 Lsun/security/x509/GeneralName;
                             1 Lsun/misc/Launcher;

      12678432     396201          0 Ljava/lang/String;
       5823928        356          0 [Ljava/lang/Object;
        464552        660          0 Ljava/lang/Class;
        124248        751          0 [B
         59024        856          0 [S
         39664        788          0 [I
         16432         83          0 [Ljava/util/HashMap$Entry;
         12064        377          0 Ljava/util/HashMap$Entry;
         10720        268          0 Ljava/util/LinkedHashMap$Entry;
         10080        315          0 Ljava/util/Hashtable$Entry;
          6912        288          0 Ljava/util/LinkedList$Entry;
          4144        259          0 Ljava/lang/Integer;
          3600         75     474416 Ljava/util/HashMap;
          3280         17          0 [Ljava/util/Hashtable$Entry;
          3160         79          0 Ljava/math/BigInteger;
          2736        114          0 Lsun/security/util/ObjectIdentifier;
          2688         48          0 Ljava/security/Provider$Service;
          2560         40          0 Ljava/net/URL;
          2488         60          0 [Ljava/lang/String;
          2448        102          0 Ljava/util/LinkedList;
          2232         93          0 Ljava/security/Provider$ServiceKey;
          1920         48          0 Ljava/util/concurrent/ConcurrentHashMap$Segment;
          1584         22          0 Ljava/lang/reflect/Constructor;
          1536         48          0 Ljava/util/concurrent/locks/ReentrantLock$NonfairSync;
          1200         30          0 Ljava/lang/ref/SoftReference;
          1192         48          0 [Ljava/util/concurrent/ConcurrentHashMap$HashEntry;
          1104         46          0 Ljava/security/Provider$UString;
          1040          1          0 [Ljava/lang/Integer;
          1000         25          0 Ljava/lang/ref/Finalizer;
           992         31          0 Ljava/util/concurrent/ConcurrentHashMap$HashEntry;
           960         30          0 Ljava/security/Provider$EngineDescription;
           896         16          0 Ljava/util/jar/JarFile;
           784          7          0 Ljava/lang/Thread;
           736         23          0 Ljava/util/Vector;
           736         23          0 Lsun/security/util/DerInputBuffer;
           736         23          0 Lsun/security/util/DerValue;
           736         23          0 Lsun/security/x509/OIDMap$OIDInfo;
           672         28          0 Ljava/io/ExpiringCache$Entry;
           640         16          0 Lsun/misc/URLClassPath$JarLoader;
           640         16          0 Ljava/io/ObjectStreamField;
           608         19          0 Ljava/util/Locale;
           592          3          0 [J
           576         25          0 [Ljava/lang/Class;
           576         12          0 Ljava/util/Hashtable;
           552         23          0 Lsun/security/util/DerInputStream;
           552         23          0 Ljava/util/ArrayList;
           504         21          0 Lsun/security/x509/RDN;
           504         21          0 Lsun/security/x509/AVAKeyword;
           504         21          0 [Lsun/security/x509/AVA;
           504         21          0 Lsun/security/x509/AVA;
           440          5          0 Ljava/lang/reflect/Method;
           408         17          0 Ljava/util/jar/Attributes$Name;
           368         23          0 Ljava/lang/Object;
           352         11          0 Lsun/security/jca/ProviderConfig;
           352         11          0 Ljava/security/AccessControlContext;
           344          3          0 [Ljava/math/BigInteger;
           336          6     208576 Ljava/util/LinkedHashMap;
           336         14          0 [Ljava/lang/Byte;
           320          4          0 Ljava/util/jar/JarFile$JarFileEntry;
           312          3          0 [D
           256         16          0 Ljava/lang/Byte;
           256          8          0 Ljava/lang/ref/WeakReference;
           256          8          0 Ljava/lang/OutOfMemoryError;
           240          3          0 [Ljava/lang/ThreadLocal$ThreadLocalMap$Entry;
           240          3          0 [Ljava/util/concurrent/ConcurrentHashMap$Segment;
           240         10          0 Lsun/reflect/NativeConstructorAccessorImpl;
           224          9          0 [Ljava/io/ObjectStreamField;
           216          9          0 Lsun/misc/JarIndex;
           192          2          0 Lsun/security/provider/Sun;
           192          4          0 Lsun/security/x509/X500Name;
           192          6          0 Lsun/security/x509/AlgorithmId;
           168          7          0 Lsun/security/x509/NetscapeCertTypeExtension$MapEntry;
           160         10          0 Lsun/reflect/DelegatingConstructorAccessorImpl;
           152          4          0 [Lsun/security/x509/RDN;
           144          2          0 Ljava/lang/reflect/Field;
           144          2          0 Lsun/nio/cs/MacRoman$Encoder;
           144          2          0 Lsun/security/x509/X509CertImpl;
           144          3     751160 Ljava/util/concurrent/ConcurrentHashMap;
           128          4          0 Ljava/lang/ThreadLocal$ThreadLocalMap$Entry;
           128          4          0 Ljava/lang/ref/ReferenceQueue;
           120          5          0 Lsun/security/jca/ServiceId;
           120          5          0 Ljava/io/FileDescriptor;
           112          2          0 Lsun/security/x509/X509CertInfo;
           112          1          0 Ljava/lang/ref/Finalizer$FinalizerThread;
           112          1          0 Ljava/lang/ref/Reference$ReferenceHandler;
           112          2          0 Lsun/security/util/MemoryCache$SoftCacheEntry;
           112          2          0 Lsun/nio/cs/StreamEncoder;
           112          2          0 Ljava/io/ExpiringCache$1;
           112          4          0 [Z
            96          3          0 Ljava/util/zip/Inflater;
            96          2          0 Ljava/nio/HeapByteBuffer;
            96          6          0 Ljava/lang/ref/ReferenceQueue$Lock;
            96          3          0 Ljava/io/FileInputStream;
            96          4          0 Ljava/io/File;
            96          1          0 Lsun/security/jca/ProviderList$1;
            96          2          0 Ljava/lang/ThreadGroup;
            96          2          0 Ljava/io/BufferedWriter;
            96          6          0 Ljava/util/concurrent/atomic/AtomicInteger;
            96          3          0 Ljava/util/Stack;
            96          4          0 Ljava/util/Date;
            96          2          0 Ljava/util/Properties;
            96          4          0 Ljava/text/Normalizer$Form;
            88          1          0 [[Ljava/lang/Byte;
            88          1          0 [Lsun/security/util/ObjectIdentifier;
            88          1          0 Ljava/util/jar/JarVerifier;
            80          1          0 [[B
            80          2          0 Lsun/security/provider/DSAPublicKeyImpl;
            80          2          0 Ljava/io/ExpiringCache;
            80          2          0 [Lsun/security/jca/ProviderConfig;
            80          1          0 Lsun/misc/Launcher$ExtClassLoader;
            80          5          0 Ljava/lang/ThreadLocal;
            80          2          0 Lsun/security/x509/AuthorityKeyIdentifierExtension;
            72          3          0 Ljava/lang/ThreadLocal$ThreadLocalMap;
            72          3          0 Lsun/misc/Signal;
            72          3          0 Ljava/util/zip/ZStreamRef;
            72          3          0 Ljava/lang/RuntimePermission;
            72          1          0 Lsun/misc/Launcher$AppClassLoader;
            64          1          0 [F
            64          2          0 Lsun/misc/URLClassPath;
            64          2          0 Ljava/security/AlgorithmParameters;
            64          1          0 Lsun/security/provider/NativePRNG$RandomIO;
            64          2          0 Lsun/security/x509/NetscapeCertTypeExtension;
            64          2          0 Ljava/io/OutputStreamWriter;
            64          4          0 Lsun/security/x509/KeyIdentifier;
            64          2          0 Lsun/security/util/MemoryCache;
            64          4          0 Ljava/util/HashMap$EntrySet;
            64          2          0 Ljava/io/PrintStream;
            64          2          0 Ljava/lang/ref/ReferenceQueue$Null;
            64          2          0 Ljava/io/FileOutputStream;
            64          2          0 Ljava/security/Permissions;
            64          2          0 [Ljava/lang/Thread;
            64          2          0 Lsun/security/x509/SubjectKeyIdentifierExtension;
            56          1          0 [Ljava/lang/Runnable;
            48          2          0 Lsun/security/x509/CertificateValidity;
            48          2   50776680 LTest$OOMList;
            48          2          0 Lsun/security/x509/CertificateIssuerName;
            48          2          0 Ljava/nio/charset/CoderResult;
            48          3          0 Lsun/text/normalizer/NormalizerBase$QuickCheckResult;
            48          2          0 Lsun/misc/NativeSignalHandler;
            48          2          0 [Ljava/io/File;
            48          2          0 Lsun/security/provider/DSAParameters;
            48          2          0 Ljava/io/BufferedOutputStream;
            48          1          0 [Lsun/security/x509/NetscapeCertTypeExtension$MapEntry;
            48          2          0 Lsun/security/x509/CertificateExtensions;
            48          2          0 Lsun/security/util/BitArray;
            48          2          0 Lsun/security/jca/ProviderList;
            48          2          0 Lsun/security/x509/CertificateSubjectName;
            48          2          0 Lsun/nio/cs/Surrogate$Parser;
            48          3          0 Ljava/nio/charset/CodingErrorAction;
            48          2          0 Lsun/security/jca/ProviderList$3;
            48          2          0 Lsun/security/util/Cache$EqualByteArray;
            40          1          0 Ljava/security/ProtectionDomain;
            40          2          0 [Ljava/security/CodeSigner;
            40          1          0 Ljava/io/BufferedInputStream;
            40          1          0 Lsun/nio/cs/StandardCharsets$Aliases;
            40          1          0 Lsun/nio/cs/StandardCharsets$Cache;
            40          1      27544 Ljava/util/IdentityHashMap;
            40          1          0 [Lsun/security/jca/ServiceId;
            40          1          0 Lsun/nio/cs/StandardCharsets$Classes;
            32          1          0 Ljava/lang/VirtualMachineError;
            32          1          0 Ljava/util/Collections$SynchronizedMap;
            32          1          0 [Ljava/lang/OutOfMemoryError;
            32          1          0 Lsun/security/x509/BasicConstraintsExtension;
            32          1          0 [Ljava/text/Normalizer$Form;
            32          2          0 Lsun/security/x509/SerialNumber;
            32          1          0 Ljava/lang/NullPointerException;
            32          1          0 Lsun/security/x509/KeyUsageExtension;
            32          1          0 Lsun/misc/SoftCache;
            32          2          0 Lsun/security/x509/CertificateVersion;
            32          2          0 Lsun/net/www/protocol/jar/Handler;
            32          2          0 Ljava/lang/Boolean;
            32          2          0 Ljava/lang/Shutdown$Lock;
            32          1          0 Ljava/security/CodeSource;
            32          1          0 [Ljava/lang/ThreadGroup;
            32          2          0 Lsun/security/x509/CertificateSerialNumber;
            32          1          0 Ljava/lang/ref/Reference;
            32          1          0 Ljava/io/UnixFileSystem;
            32          1          0 Lsun/security/x509/SubjectAlternativeNameExtension;
            32          1          0 Ljava/lang/ArithmeticException;
            32          1          0 Ljava/lang/ClassLoader$NativeLibrary;
            32          1          0 Ljava/io/FilePermission;
            32          2          0 Lsun/security/x509/CertificateX509Key;
            32          2          0 Ljava/util/HashSet;
            32          1          0 Ljava/util/concurrent/atomic/AtomicReferenceFieldUpdater$AtomicReferenceFieldUpdaterImpl;
            32          1          0 Lsun/nio/cs/StandardCharsets;
            32          2          0 Lsun/security/x509/CertificateAlgorithmId;
            32          2          0 Ljava/nio/ByteOrder;
            32          1          0 Ljava/security/BasicPermissionCollection;
            24          1          0 Lcom/apple/java/Usage;
            24          1          0 Ljava/lang/reflect/ReflectPermission;
            24          1          0 Lsun/misc/URLClassPath$FileLoader;
            24          1          0 Ljava/util/Collections$UnmodifiableRandomAccessList;
            24          1          0 Lsun/nio/cs/US_ASCII;
            24          1          0 Ljava/math/MutableBigInteger;
            24          1          0 Ljava/util/Collections$EmptyMap;
            24          1          0 Lsun/security/provider/certpath/X509CertPath;
            24          1          0 Ljava/security/Policy$UnsupportedEmptyCollection;
            24          1          0 Ljava/security/CodeSigner;
            24          1          0 Lsun/nio/cs/UTF_8;
            24          1          0 Ljava/io/FilePermissionCollection;
            24          1          0 Ljava/lang/StringBuilder;
            24          1          0 Ljava/util/Arrays$ArrayList;
            24          1          0 Lsun/nio/cs/MacRoman;
            24          1          0 Ljava/util/BitSet;
            16          1          0 [Ljava/security/Provider;
            16          1          0 Lsun/jkernel/DownloadManager$1;
            16          1          0 Lsun/text/normalizer/NormalizerBase$NFKCMode;
            16          1          0 Lsun/misc/Launcher$Factory;
            16          1          0 Ljava/nio/charset/CoderResult$2;
            16          1          0 Lsun/net/www/protocol/file/Handler;
            16          1          0 Ljava/util/jar/JavaUtilJarAccessImpl;
            16          1          0 Ljava/lang/System$2;
            16          1          0 Ljava/lang/reflect/ReflectAccess;
            16          1          0 Lsun/misc/FloatingDecimal$1;
            16          1          0 Ljava/util/jar/JarVerifier$3;
            16          1          0 Lsun/text/normalizer/NormalizerBase$Mode;
            16          1          0 Ljava/util/Collections$EmptySet;
            16          1          0 Ljava/util/Hashtable$EmptyEnumerator;
            16          1          0 Lsun/misc/ASCIICaseInsensitiveComparator;
            16          1          0 [Ljava/security/cert/Certificate;
            16          1          0 Ljava/security/ProtectionDomain$Key;
            16          1          0 Lcom/apple/java/Usage$1;
            16          1          0 Ljava/lang/Runtime;
            16          1          0 Ljava/security/AccessControlContext$1;
            16          1          0 [Ljava/lang/StackTraceElement;
            16          1          0 Lsun/text/normalizer/NormalizerBase$NFDMode;
            16          1          0 Ljava/util/Collections$UnmodifiableCollection;
            16          1          0 Ljava/util/Collections$EmptyList;
            16          1          0 Ljava/lang/InheritableThreadLocal;
            16          1          0 Ljava/nio/charset/CoderResult$1;
            16          1          0 Lsun/security/x509/RFC822Name;
            16          1          0 Lsun/reflect/ReflectionFactory;
            16          1          0 Ljava/lang/Terminator$1;
            16          1          0 Ljava/lang/ApplicationShutdownHooks$1;
            16          1          0 Lsun/security/util/ByteArrayLexOrder;
            16          1          0 Ljava/net/URLClassLoader$7;
            16          1          0 [Ljava/security/Principal;
            16          1          0 Lsun/security/util/ByteArrayTagOrder;
            16          1          0 Lsun/security/x509/GeneralName;
            16          1          0 Ljava/util/regex/Pattern$5;
            16          1          0 Ljava/lang/String$CaseInsensitiveComparator;
            16          1          0 Lsun/util/calendar/Gregorian;
            16          1          0 Lsun/security/x509/GeneralNames;
            16          1          0 Lsun/misc/Launcher;
            16          1          0 Ljava/util/regex/Pattern$Node;
            16          1          0 Ljava/lang/ref/Reference$Lock;
            16          1          0 Lsun/text/normalizer/NormalizerBase$NFCMode;
            16          1          0 Ljava/util/Collections$ReverseComparator;
            16          1          0 Lsun/misc/Unsafe;
            16          1          0 Lsun/text/normalizer/NormalizerBase$NFKDMode;
            16          1          0 Ljava/util/Hashtable$EmptyIterator;
            16          1          0 Ljava/security/ProtectionDomain$2;
            16          1          0 Ljava/net/UnknownContentHandler;
            16          1          0 Ljava/io/FileDescriptor$1;
            16          1          0 Ljava/util/regex/Pattern$LastNode;
    ---------- ---------- ----------------------

    Resuming threads.
    Printing thread dump.

    Dumping thread state for 4 threads

    #1 - Signal Dispatcher - RUNNABLE

    #2 - Finalizer - WAITING
            at java.lang.Object.wait(Object.java)
            at java.lang.ref.ReferenceQueue.remove(ReferenceQueue.java:118)
            at java.lang.ref.ReferenceQueue.remove(ReferenceQueue.java:134)
            at java.lang.ref.Finalizer$FinalizerThread.run(Finalizer.java:159)

    #3 - Reference Handler - WAITING
            at java.lang.Object.wait(Object.java)
            at java.lang.Object.wait(Object.java:485)
            at java.lang.ref.Reference$ReferenceHandler.run(Reference.java:116)

    #4 - main - RUNNABLE - [OOM thrower]
            at java.lang.StringBuilder.toString(StringBuilder.java:430)
            at Test.main(Test.java:16)


### License

Polarbear is based on Sun Microsystems sample JVMTI code, and follows the license rules issued by Sun.
All modifications are released under the same license.


### Polarbear?

If you were a fan of LOST, you may understand why "Out of memory" and polar bears are related thoughts for us.
