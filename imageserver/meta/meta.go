package meta

const (
	TAG       = "tag_"
	SPLIT     = "/"
	DIRECTORY = "DIRECTORY_"
)

//TODO
//groupId == 0 or fileId == 0
//may cause error
type MetaInfoValue struct {
	Index   uint64
	Start   uint64
	End     uint64
	GroupId uint16 `json:",omitempty"`
	FileId  uint64 `json:",omitempty"`
	IsLast  bool
}

type MetaInfo struct {
	Path  string
	Value *MetaInfoValue
}

type MetaDriver interface {
	StoreMetaInfo(metaInfo *MetaInfo) error
	DeleteFileMetaInfo(path string) error
	GetDirectoryInfo(path string) ([]string, error)
	GetFileMetaInfo(path string, detail bool) ([]*MetaInfoValue, error)
	GetFragmentMetaInfo(path string, index, start, end uint64) (*MetaInfoValue, error)
}
