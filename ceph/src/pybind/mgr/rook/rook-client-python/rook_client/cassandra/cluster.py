"""
This file is automatically generated.
Do not modify.
"""

try:
    from typing import Any, Optional, Union, List
except ImportError:
    pass

from .._helper import _omit, CrdObject, CrdObjectList, CrdClass

class Storage(CrdObject):
    _properties = [
        ('volumeClaimTemplates', 'volumeClaimTemplates', object, True, False)
    ]        

    def __init__(self,
                 volumeClaimTemplates,  # type: Any
                 ):
        super(Storage, self).__init__(
            volumeClaimTemplates=volumeClaimTemplates,
        )

    @property
    def volumeClaimTemplates(self):
        # type: () -> Any
        return self._property_impl('volumeClaimTemplates')
    
    @volumeClaimTemplates.setter
    def volumeClaimTemplates(self, new_val):
        # type: (Any) -> None
        self._volumeClaimTemplates = new_val


class Resources(CrdObject):
    _properties = [
        ('cassandra', 'cassandra', object, False, False),
        ('sidecar', 'sidecar', object, False, False)
    ]        

    def __init__(self,
                 cassandra=_omit,  # type: Optional[Any]
                 sidecar=_omit,  # type: Optional[Any]
                 ):
        super(Resources, self).__init__(
            cassandra=cassandra,
            sidecar=sidecar,
        )

    @property
    def cassandra(self):
        # type: () -> Any
        return self._property_impl('cassandra')
    
    @cassandra.setter
    def cassandra(self, new_val):
        # type: (Optional[Any]) -> None
        self._cassandra = new_val
    
    @property
    def sidecar(self):
        # type: () -> Any
        return self._property_impl('sidecar')
    
    @sidecar.setter
    def sidecar(self, new_val):
        # type: (Optional[Any]) -> None
        self._sidecar = new_val


class RacksItem(CrdObject):
    _properties = [
        ('name', 'name', str, True, False),
        ('members', 'members', int, True, False),
        ('configMapName', 'configMapName', str, False, False),
        ('storage', 'storage', Storage, True, False),
        ('placement', 'placement', object, False, False),
        ('resources', 'resources', Resources, True, False),
        ('sidecarImage', 'sidecarImage', object, False, False)
    ]        

    def __init__(self,
                 name,  # type: str
                 members,  # type: int
                 storage,  # type: Storage
                 resources,  # type: Resources
                 configMapName=_omit,  # type: Optional[str]
                 placement=_omit,  # type: Optional[Any]
                 sidecarImage=_omit,  # type: Optional[Any]
                 ):
        super(RacksItem, self).__init__(
            name=name,
            members=members,
            storage=storage,
            resources=resources,
            configMapName=configMapName,
            placement=placement,
            sidecarImage=sidecarImage,
        )

    @property
    def name(self):
        # type: () -> str
        return self._property_impl('name')
    
    @name.setter
    def name(self, new_val):
        # type: (str) -> None
        self._name = new_val
    
    @property
    def members(self):
        # type: () -> int
        return self._property_impl('members')
    
    @members.setter
    def members(self, new_val):
        # type: (int) -> None
        self._members = new_val
    
    @property
    def configMapName(self):
        # type: () -> str
        return self._property_impl('configMapName')
    
    @configMapName.setter
    def configMapName(self, new_val):
        # type: (Optional[str]) -> None
        self._configMapName = new_val
    
    @property
    def storage(self):
        # type: () -> Storage
        return self._property_impl('storage')
    
    @storage.setter
    def storage(self, new_val):
        # type: (Storage) -> None
        self._storage = new_val
    
    @property
    def placement(self):
        # type: () -> Any
        return self._property_impl('placement')
    
    @placement.setter
    def placement(self, new_val):
        # type: (Optional[Any]) -> None
        self._placement = new_val
    
    @property
    def resources(self):
        # type: () -> Resources
        return self._property_impl('resources')
    
    @resources.setter
    def resources(self, new_val):
        # type: (Resources) -> None
        self._resources = new_val
    
    @property
    def sidecarImage(self):
        # type: () -> Any
        return self._property_impl('sidecarImage')
    
    @sidecarImage.setter
    def sidecarImage(self, new_val):
        # type: (Optional[Any]) -> None
        self._sidecarImage = new_val


class RacksList(CrdObjectList):
    _items_type = RacksItem


class Datacenter(CrdObject):
    _properties = [
        ('name', 'name', str, True, False),
        ('racks', 'racks', RacksList, False, False)
    ]        

    def __init__(self,
                 name,  # type: str
                 racks=_omit,  # type: Optional[Union[List[RacksItem], CrdObjectList]]
                 ):
        super(Datacenter, self).__init__(
            name=name,
            racks=racks,
        )

    @property
    def name(self):
        # type: () -> str
        return self._property_impl('name')
    
    @name.setter
    def name(self, new_val):
        # type: (str) -> None
        self._name = new_val
    
    @property
    def racks(self):
        # type: () -> Union[List[RacksItem], CrdObjectList]
        return self._property_impl('racks')
    
    @racks.setter
    def racks(self, new_val):
        # type: (Optional[Union[List[RacksItem], CrdObjectList]]) -> None
        self._racks = new_val


class Spec(CrdObject):
    _properties = [
        ('version', 'version', str, True, False),
        ('datacenter', 'datacenter', Datacenter, True, False)
    ]        

    def __init__(self,
                 version,  # type: str
                 datacenter,  # type: Datacenter
                 ):
        super(Spec, self).__init__(
            version=version,
            datacenter=datacenter,
        )

    @property
    def version(self):
        # type: () -> str
        return self._property_impl('version')
    
    @version.setter
    def version(self, new_val):
        # type: (str) -> None
        self._version = new_val
    
    @property
    def datacenter(self):
        # type: () -> Datacenter
        return self._property_impl('datacenter')
    
    @datacenter.setter
    def datacenter(self, new_val):
        # type: (Datacenter) -> None
        self._datacenter = new_val


class Cluster(CrdClass):
    _properties = [
        ('apiVersion', 'apiVersion', str, True, False),
        ('metadata', 'metadata', object, True, False),
        ('status', 'status', object, False, False),
        ('spec', 'spec', Spec, True, False)
    ]        

    def __init__(self,
                 apiVersion,  # type: str
                 metadata,  # type: Any
                 spec,  # type: Spec
                 status=_omit,  # type: Optional[Any]
                 ):
        super(Cluster, self).__init__(
            apiVersion=apiVersion,
            metadata=metadata,
            spec=spec,
            status=status,
        )

    @property
    def apiVersion(self):
        # type: () -> str
        return self._property_impl('apiVersion')
    
    @apiVersion.setter
    def apiVersion(self, new_val):
        # type: (str) -> None
        self._apiVersion = new_val
    
    @property
    def metadata(self):
        # type: () -> Any
        return self._property_impl('metadata')
    
    @metadata.setter
    def metadata(self, new_val):
        # type: (Any) -> None
        self._metadata = new_val
    
    @property
    def status(self):
        # type: () -> Any
        return self._property_impl('status')
    
    @status.setter
    def status(self, new_val):
        # type: (Optional[Any]) -> None
        self._status = new_val
    
    @property
    def spec(self):
        # type: () -> Spec
        return self._property_impl('spec')
    
    @spec.setter
    def spec(self, new_val):
        # type: (Spec) -> None
        self._spec = new_val
