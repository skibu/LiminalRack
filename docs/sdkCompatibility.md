# SDK Compatibility

The complication is that the third-party plugins have been compiled against the official V2 
version of the Rack SDK. We don't have control over the compiling of those plugins. That
means that they have to be compatible with the changed Liminal Rack SDK. Yet want to make changes
to the Liminal software. That means the changes to the Rack SDK have to be compatible with the  
Rack SDK. 

There are four potential issues with modifying classes. Need to only be concerned about classes 
that are used by plugin software. The most important classes are Widget and its subclasses. If a
class is instead just for internal use and won't be accessed directly by a plugin 

## Class names
- CANNOT change class names. The linker uses them to link the plugin software to the Rack SDK.

## Non-virtual functions
- CAN change order of them or add ones since they are accessed by name by the linker.
- CANNOT change function names or parameter types for functions used by plugins
  since they are accessed by name by the linker. You get a Symbol not found error in the logfile then. The symbol name will be a
  mangled combination of the function name and the parameters. You can just google the symbol name
  to get the details. 
- CANNOT remove some functions since linker will try to find them. This happens when the plugins are
  loaded at runtime. You get a Symbol not found error in the logfile then. The symbol name will be a
  mangled combination of the function name and the parameters. You can just google the symbol name
  to get the details.
- CANNOT change const-ness of non-virtual functions since they are accessed by the mangled name which
  includes const-ness. But can add functions so can easily duplicate a non-const function as also
  being a const one.

## Virtual functions
- CANNOT change order of the virtual functions relative to each other. This is due to the vtable
  listing tbe functions in order. They are not reference by name by the plugins but instead just
  by their order.
- CANNOT add or remove virtual functions since that will change the functions in the virtual table
  and then the wrong function can be called. Cannot even add a virtual function to the end of a class
  since then the vtable will be offset for any subclasses.
- CAN change name of virtual functions since they are accessed by their order in the vtable. But
  recommend against this because it could be confusing.
- CAN change names of virtual function params.

## Data members
- CAN change names of data members. Especially nice since should change member data names to have
  '_' suffix and then add non-virtual member functions to access them.
- CAN change data members inside a Internal data member for classes that use the Pimpl Idiom. That
  is the main reason for using the Pimpl Idiom in the first place. It allows the internals of
  the contained data members to be completely hidden from the plugin software.
- CAN change accessibility, whether members are public, private, or protected. This is because
  plugins access the SDK through the linker which doesn't care about accessibility.
- CANNOT change order of data members since they are accessed by where they are relative to the
  start of the object.
- CANNOT add or remove data members since plugin software accesses them by their location within
  the object. The exception is that you can add a member to the end of a final class since
  there won't be any subclass data members to get screwed up.
- CANNOT change type of data members since that affects their size which in turn affects their
  location relative to the beginning of the object. So don't change floats to doubles for example.
  
